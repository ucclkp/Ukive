// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of necro project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "necro/layout_processor.h"

#include <fstream>

#include "utils/log.h"
#include "utils/xml/xml_parser.h"
#include "utils/xml/xml_writer.h"
#include "utils/convert.h"
#include "utils/endian.hpp"
#include "utils/number.hpp"
#include "utils/string_utils.h"

#include "necro/layout_constants.h"


namespace necro {

    const char16_t kHistoryFileName[] = u"necro_histories";

    LayoutProcessor::LayoutProcessor()
        : need_second_(false),
          cur_view_id_(10000),
          cur_layout_id_(10000) {}

    bool LayoutProcessor::process(
        const fs::path& res_dir, const fs::path& out_dir, bool* changed)
    {
        std::error_code ec;
        std::vector<fs::directory_entry> xml_files;
        for (auto& f : fs::directory_iterator(res_dir, ec)) {
            if (!f.is_directory(ec) && utl::toASCIILower(f.path().extension().u16string()) == u".xml") {
                xml_files.push_back(f);
            }
        }

        if (xml_files.empty()) {
            LOG(Log::INFO) << "Cannot find xml files";
        } else {
            LOG(Log::INFO) << xml_files.size() << " xml file(s) found";

            std::sort(
                xml_files.begin(), xml_files.end(),
                [](const fs::directory_entry& d1, const fs::directory_entry& d2) -> bool
            {
                return d1.path() < d2.path();
            });
        }

        // 从文件中读出历史
        std::vector<History> histories;
        readHistory(out_dir, &histories);

        // 看看历史中有没有相同记录
        *changed = false;
        for (const auto& xml_file : xml_files) {
            bool hit = false;
            for (auto it = histories.begin(); it != histories.end(); ++it) {
                if (it->xml_path == xml_file.path()) {
                    uint64_t last_write = std::chrono::duration_cast<std::chrono::microseconds>(
                        xml_file.last_write_time(ec).time_since_epoch()).count();
                    if (!ec && it->ts == last_write)
                    {
                        hit = true;
                        histories.erase(it);
                        break;
                    }
                }
            }
            if (!hit) {
                *changed = true;
                break;
            }
        }
        if (!histories.empty()) {
            *changed = true;
        }

        // 所有 XML 文件都没有改动，就直接返回
        if (!*changed) {
            std::vector<fs::directory_entry> out_xml_files;
            for (auto& f : fs::directory_iterator(out_dir, ec)) {
                if (!f.is_directory(ec) && utl::toASCIILower(f.path().extension().u16string()) == u".xml") {
                    out_xml_files.push_back(f);
                }
            }

            for (const auto& xml_file : xml_files) {
                for (auto it = out_xml_files.begin(); it != out_xml_files.end(); ++it) {
                    if (it->path().filename() == xml_file.path().filename()) {
                        out_xml_files.erase(it);
                        break;
                    }
                }
            }

            for (const auto& xml_file : out_xml_files) {
                fs::remove(xml_file.path(), ec);
            }

            LOG(Log::INFO) << "No xml files were changed.";
            return true;
        }

        fs::remove(out_dir, ec);

        // 处理 XML 文件
        for (const auto& xml_file : xml_files) {
            LOG(Log::INFO) << "Processing xml file: " << xml_file.path().u8string();

            std::ifstream reader(xml_file.path(), std::ios::binary);
            if (!reader) {
                LOG(Log::ERR) << "Cannot open xml file: " << xml_file.path().filename().u8string();
                return false;
            }

            utl::XMLParser xml_parser;
            std::shared_ptr<utl::xml::Element> root;
            if (!xml_parser.parse(reader, &root)) {
                auto& pedometer = xml_parser.getPedometer();
                LOG(Log::ERR) << "Failed to parse xml file: " << xml_file.path().filename().u8string()
                    << " line: " << pedometer.getCurRow() << " col: " << pedometer.getCurCol();
                return false;
            }

            IdMap cur_map;
            if (!traverseTree(root, true, &cur_map)) {
                LOG(Log::ERR) << "Failed to traverse xml file: " << xml_file.path().filename().u8string();
                return false;
            }

            if (need_second_) {
                if (!traverseTree(root, false, &cur_map)) {
                    LOG(Log::ERR) << "Failed to traverse xml file: " << xml_file.path().filename().u8string();
                    return false;
                }
            }

            for (const auto& cur : cur_map) {
                view_id_map_.insert(cur);
            }
            cur_map.clear();

            std::string xml_str;
            utl::XMLWriter xml_writer;
            if (!xml_writer.write(*root, &xml_str)) {
                LOG(Log::ERR) << "Failed to write xml file: " << xml_file.path().filename().u8string();
                return false;
            }

            fs::path new_file = out_dir / xml_file.path().filename();
            if (!fs::create_directories(out_dir, ec) && ec) {
                LOG(Log::ERR) << "Failed to make dir: " << out_dir.u8string();
                return false;
            }
            std::ofstream writer(new_file, std::ios::binary | std::ios::ate);
            if (!writer) {
                LOG(Log::ERR) << "Cannot open file: " << new_file.u8string();
                return false;
            }

            writer.write(xml_str.data(), xml_str.length());

            auto xml_file_name = xml_file.path().filename().u8string();
            if (!xml_file_name.empty()) {
                layout_id_map_[xml_file_name] = cur_layout_id_;
                ++cur_layout_id_;
            }
        }

        LOG(Log::INFO) << "Generating layout id file...";

        // 生成包含布局 id 和文件名对应关系的记录文件
        if (!layout_id_map_.empty()) {
            std::string out_map_str;
            for (const auto& pair : layout_id_map_) {
                out_map_str.append(std::to_string(pair.second))
                    .append("=").append(pair.first).append("\n");
            }
            std::ofstream writer(out_dir / kLayoutIdFileName, std::ios::binary | std::ios::ate);
            writer.write(out_map_str.data(), out_map_str.length());
            if (!writer) {
                LOG(Log::ERR) << "Failed to write layout id file.";
                return false;
            }
        } else {
            LOG(Log::INFO) << "Layout id map is empty, no file generated";
        }

        // 生成历史，写入文件
        histories.clear();
        for (const auto& xml_file : xml_files) {
            uint64_t last_write = std::chrono::duration_cast<std::chrono::microseconds>(
                xml_file.last_write_time(ec).time_since_epoch()).count();
            if (!ec) {
                History his;
                his.xml_path = xml_file;
                his.ts = last_write;
                histories.push_back(std::move(his));
            }
        }
        writeHistory(out_dir, histories);

        return true;
    }

    const LayoutProcessor::IdMap& LayoutProcessor::getViewIdMap() const {
        return view_id_map_;
    }

    const LayoutProcessor::IdMap& LayoutProcessor::getLayoutIdMap() const {
        return layout_id_map_;
    }

    void LayoutProcessor::readHistory(
        const fs::path& out_dir, std::vector<History>* histories)
    {
        std::ifstream cache_file(out_dir / kHistoryFileName, std::ios::binary);
        if (!cache_file) {
            return;
        }
        std::istream& s = cache_file;

        uint8_t available;
        s.read(reinterpret_cast<char*>(&available), 1);
        if (!s.good()) {
            return;
        }
        if (available == 0) {
            return;
        }

        uint32_t count;
        s.read(reinterpret_cast<char*>(&count), 4);
        if (!s.good()) {
            return;
        }
        count = utl::fromToBE(count);
        for (uint32_t i = 0; i < count; ++i) {
            History his;
            std::string path_u8;
            for (;;) {
                char buf;
                s.read(&buf, 1);
                if (!s.good() || buf == 0) break;
                path_u8.push_back(buf);
            }
            his.xml_path = utl::UTF8ToUTF16(path_u8);
            s.read(reinterpret_cast<char*>(&his.ts), 8);
            if (!s.good()) break;
            his.ts = utl::fromToBE(his.ts);

            histories->push_back(std::move(his));
        }
    }

    void LayoutProcessor::writeHistory(
        const fs::path& out_dir, const std::vector<History>& histories)
    {
        if (histories.empty()) {
            return;
        }

        std::ofstream cache_file(out_dir / kHistoryFileName, std::ios::binary | std::ios::trunc);
        if (!cache_file) {
            return;
        }
        std::ostream& s = cache_file;

        uint8_t available = 0;
        s.write(reinterpret_cast<const char*>(&available), 1);
        if (!s.good()) {
            return;
        }

        uint32_t count = utl::fromToBE(utl::num_cast<uint32_t>(histories.size()));
        s.write(reinterpret_cast<const char*>(&count), 4);
        if (!s.good()) {
            return;
        }
        for (const auto& item : histories) {
            auto path_u8 = item.xml_path.u8string();
            s.write(reinterpret_cast<const char*>(path_u8.data()), path_u8.size());
            if (!s.good()) { return; }

            uint8_t term = 0;
            s.write(reinterpret_cast<const char*>(&term), 1);
            if (!s.good()) { return; }

            uint64_t ts = utl::fromToBE(item.ts);
            s.write(reinterpret_cast<const char*>(&ts), 8);
            if (!s.good()) { return; }
        }
    }

    bool LayoutProcessor::traverseTree(const ElementPtr& element, bool is_first, IdMap* cur_map) {
        if (!element) {
            return false;
        }

        for (auto& attr : element->attrs) {
            auto attr_val = attr.second;
            if (utl::ascii::startWith(attr_val, "@+id/")) {
                auto id_val = attr_val.substr(5);
                if (id_val.empty()) {
                    LOG(Log::ERR) << "The id attr: " << attr.first
                        << " of element: " << element->tag_name
                        << " is invalid";
                    return false;
                }

                // 当前文件中的 view_id 重复
                if (cur_map->find(id_val) != cur_map->end()) {
                    LOG(Log::ERR) << "The id: " << id_val
                        << " of element: " << element->tag_name
                        << " is duplicated";
                    return false;
                }
                // 整个项目的 view_id 重复
                if (view_id_map_.find(id_val) != view_id_map_.end()) {
                    LOG(Log::ERR) << "The id: " << id_val
                        << " of element: " << element->tag_name
                        << " is duplicated in other file.";
                    return false;
                }

                cur_map->insert({ id_val, cur_view_id_ });
                attr.second = std::to_string(cur_view_id_);
                ++cur_view_id_;
            } else {
                bool modified = false;
                decltype(attr_val)::size_type cur_idx = 0;
                for (;;) {
                    auto idx = attr_val.find('@', cur_idx);
                    if (idx == decltype(attr_val)::npos) {
                        if (modified) {
                            attr.second = attr_val;
                        }
                        break;
                    }

                    if (!utl::ascii::startWith(attr_val, "@id/", idx)) {
                        LOG(Log::ERR) << "Unsupported @ operation in: " << attr_val
                            << " of element: " << element->tag_name;
                        return false;
                    }

                    auto end_idx = attr_val.find(',', idx);
                    if (end_idx == decltype(attr_val)::npos) {
                        end_idx = attr_val.length();
                    }

                    auto id_val = utl::ascii::trim(attr_val.substr(idx + 4, end_idx - idx - 4), false);
                    if (id_val.empty()) {
                        LOG(Log::ERR) << "The id in attr: " << attr.first
                            << " of element: " << element->tag_name
                            << " is invalid.";
                        return false;
                    }

                    auto it = cur_map->find(id_val);
                    if (it == cur_map->end()) {
                        if (is_first) {
                            need_second_ = true;
                            cur_idx = end_idx;
                        } else {
                            LOG(Log::ERR) << "Cannot find id: " << id_val
                                << " in attr: " << attr.first
                                << " of element: " << element->tag_name;
                            return false;
                        }
                    } else {
                        attr_val.replace(idx, end_idx - idx, std::to_string(it->second));
                        cur_idx = idx;
                        modified = true;
                    }
                }
            }
        }

        for (const auto& content : element->contents) {
            if (content.type != utl::xml::Content::Type::Element) {
                continue;
            }
            if (!traverseTree(content.element, is_first, cur_map)) {
                return false;
            }
        }

        return true;
    }

}
