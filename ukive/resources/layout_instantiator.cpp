// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "ukive/resources/layout_instantiator.h"

#include <fstream>

#include "utils/convert.h"
#include "utils/log.h"
#include "utils/xml/xml_parser.h"
#include "utils/files/file_utils.h"
#include "utils/string_utils.h"

#include "ukive/views/layout/layout_view.h"

#include "ukive/app/application.h"
#include "ukive/diagnostic/grid_view.h"
#include "ukive/resources/resource_manager.h"
#include "ukive/views/button.h"
#include "ukive/views/chart_view.h"
#include "ukive/views/check_box.h"
#include "ukive/views/combo_box.h"
#include "ukive/views/image_view.h"
#include "ukive/views/progress_bar.h"
#include "ukive/views/radio_button.h"
#include "ukive/views/scroll_view.h"
#include "ukive/views/seek_bar.h"
#include "ukive/views/switch_view.h"
#include "ukive/views/list/list_view.h"
#include "ukive/views/layout/title_bar_layout.h"
#include "ukive/views/layout/simple_layout.h"
#include "ukive/views/layout/sequence_layout.h"
#include "ukive/views/layout/non_client_layout.h"
#include "ukive/views/layout/restraint_layout.h"
#include "ukive/views/layout/root_layout.h"
#include "ukive/views/tab/tab_strip_view.h"
#include "ukive/views/tab/tab_view.h"

#include "necro/layout_constants.h"


#define VIEW_CONSTRUCTOR(name)  \
    handler_map_[#name] = [](Context c, const Attributes& attrs)->View* {  \
        return new name(c, attrs);  \
    };


namespace ukive {

    std::map<std::string, LayoutInstantiator::Handler> LayoutInstantiator::handler_map_;

    LayoutInstantiator::LayoutInstantiator()
        : has_read_lim_(false),
          root_parent_(nullptr) {}

    // static
    void LayoutInstantiator::init() {
        VIEW_CONSTRUCTOR(View);
        VIEW_CONSTRUCTOR(Button);
        VIEW_CONSTRUCTOR(ChartView);
        VIEW_CONSTRUCTOR(CheckBox);
        VIEW_CONSTRUCTOR(RadioButton);
        VIEW_CONSTRUCTOR(ProgressBar);
        VIEW_CONSTRUCTOR(ImageView);
        VIEW_CONSTRUCTOR(SeekBar);
        VIEW_CONSTRUCTOR(ComboBox);
        VIEW_CONSTRUCTOR(SwitchView);
        VIEW_CONSTRUCTOR(TextView);
        VIEW_CONSTRUCTOR(GridView);

        VIEW_CONSTRUCTOR(LayoutView);
        VIEW_CONSTRUCTOR(TitleBarLayout);
        VIEW_CONSTRUCTOR(SimpleLayout);
        VIEW_CONSTRUCTOR(SequenceLayout);
        VIEW_CONSTRUCTOR(ListView);
        VIEW_CONSTRUCTOR(NonClientLayout);
        VIEW_CONSTRUCTOR(RestraintLayout);
        VIEW_CONSTRUCTOR(RootLayout);
        VIEW_CONSTRUCTOR(ScrollView);
        VIEW_CONSTRUCTOR(TabStripView);
        VIEW_CONSTRUCTOR(TabView);
    }

    // static
    View* LayoutInstantiator::from(Context c, LayoutView* parent, int layout_id) {
        return LayoutInstantiator().instantiate(c, parent, layout_id);
    }

    View* LayoutInstantiator::instantiate(Context c, LayoutView* parent, int layout_id) {
        context_ = c;
        root_parent_ = parent;

        std::filesystem::path xml_file_path;
        if (!fetchLayoutFileName(layout_id, &xml_file_path)) {
            return nullptr;
        }

        std::ifstream reader(xml_file_path, std::ios::binary);
        if (!reader) {
            DCHECK(false);
            return nullptr;
        }

        utl::XMLParser parser;
        std::shared_ptr<utl::XMLParser::Element> root;
        if (!parser.parse(reader, &root)) {
            DCHECK(false);
            return nullptr;
        }

        View* root_view = nullptr;
        if (!traverseTree(root, &root_view)) {
            DCHECK(false);
            return nullptr;
        }

        return root_view;
    }

    bool LayoutInstantiator::fetchLayoutFileName(int layout_id, std::filesystem::path* file_name) {
        DCHECK(file_name != nullptr);

        if (!has_read_lim_) {
            has_read_lim_ = true;

            auto rm = Application::getResourceManager();
            auto file_path = rm->getResRootPath();
            file_path.append(u"necro");
            auto lm_file_path = file_path / necro::kLayoutIdFileName;

            std::ifstream id_file_reader(lm_file_path, std::ios::in | std::ios::binary);
            if (!id_file_reader) {
                DCHECK(false);
                return false;
            }

            char buf;
            std::string str;
            bool prev_r = false;
            std::vector<std::string> lines;
            for (;;) {
                id_file_reader.read(&buf, 1);
                if (id_file_reader.bad()) {
                    DCHECK(false);
                    return false;
                }
                if (id_file_reader.eof()) {
                    break;
                }

                if (buf == '\r') {
                    lines.push_back(str);
                    str.clear();
                    prev_r = true;
                } else if (buf == '\n') {
                    if (!prev_r) {
                        lines.push_back(str);
                        str.clear();
                    }
                    prev_r = false;
                } else {
                    str.push_back(buf);
                    prev_r = false;
                }
            }

            for (const auto& line : lines) {
                if (line.empty()) {
                    continue;
                }

                auto pair = utl::ascii::split(line, "=");
                if (pair.size() != 2) {
                    continue;
                }

                uint32_t id;
                if (!utl::stringToNumber(pair[0], &id)) {
                    DCHECK(false);
                    return false;
                }

                layout_id_map_[id] = file_path / utl::UTF8ToUTF16(pair[1]);
            }
        }

        auto it = layout_id_map_.find(layout_id);
        if (it == layout_id_map_.end()) {
            return false;
        }

        *file_name = it->second;
        return true;
    }

    bool LayoutInstantiator::traverseTree(const ElementPtr& element, View** parent) {
        DCHECK(parent != nullptr);

        if (!element) {
            return false;
        }

        if (*parent && !(*parent)->isLayoutView()) {
            LOG(Log::ERR) << "The parent of the View: " << element->tag_name << " is not a LayoutView.";
            return false;
        }

        View* cur_view;
        if (element->tag_name == "RadioGroup") {
            if (!*parent) {
                LOG(Log::ERR) << element->tag_name << " cannot be root!";
                return false;
            }
            RadioButton::StartGroup();
            cur_view = *parent;
        } else {
            auto it = handler_map_.find(element->tag_name);
            if (it == handler_map_.end()) {
                LOG(Log::ERR) << "Cannot find View: " << element->tag_name;
                return false;
            }

            auto view_constructor = it->second;
            cur_view = view_constructor(context_, element->attrs);
            if (!*parent) {
                *parent = cur_view;
                auto lp = root_parent_->makeExtraLayoutInfo(element->attrs);
                cur_view->setExtraLayoutInfo(lp);
            } else {
                auto lv = static_cast<LayoutView*>(*parent);
                auto lp = lv->makeExtraLayoutInfo(element->attrs);
                cur_view->setExtraLayoutInfo(lp);
                lv->addView(cur_view);
            }
        }

        for (const auto& content : element->contents) {
            if (content.type != utl::xml::Content::Type::Element) {
                continue;
            }
            if (!traverseTree(content.element, &cur_view)) {
                return false;
            }
        }

        if (element->tag_name == "RadioGroup") {
            RadioButton::EndGroup();
        }

        return true;
    }

}
