// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "ukive/app/application.h"

#include "utils/log.h"
#include "utils/command_line.h"
#include "utils/message/message.h"
#include "utils/message/message_pump.h"

#include "ukive/graphics/display.h"
#include "ukive/graphics/graphic_device_manager.h"
#include "ukive/graphics/images/lc_image_factory.h"
#include "ukive/text/input_method_manager.h"
#include "ukive/resources/layout_instantiator.h"
#include "ukive/resources/resource_manager.h"


namespace ukive {

    int Application::view_uid_ = 10;
    bool Application::vsync_enabled_ = true;
    Application* Application::instance_ = nullptr;


    Application::Application(const Options& options)
        : options_(options)
    {
        utl::CommandLine::initialize();
        initApplication();
        instance_ = this;
    }

    Application::Application(const Options& options, int argc, char16_t* argv[])
        : options_(options)
    {
        utl::CommandLine::initialize(argc, argv);
        initApplication();
        instance_ = this;
    }

    Application::~Application() {
        cleanApplication();
        instance_ = nullptr;
    }

    void Application::initApplication() {
        initPlatform();

        utl::MessagePump::createMain();

        LayoutInstantiator::init();

        gdm_.reset(GraphicDeviceManager::create());
        if (!gdm_->initialize()) {
            LOG(Log::FATAL) << "Failed to initialize GraphicDeviceManager";
            return;
        }

        ilf_.reset(LcImageFactory::create());
        bool ret = ilf_->initialization();
        if (!ret) {
            LOG(Log::ERR) << "Failed to initialize LcImageFactory";
        }

        imm_.reset(InputMethodManager::create());
        ret = imm_->initialization();
        if (!ret) {
            LOG(Log::ERR) << "Failed to initialize InputMethodManager";
        }

        res_mgr_.reset(new ResourceManager());
        ret = res_mgr_->initialize();
        if (!ret) {
            LOG(Log::ERR) << "Failed to initialize ResourceManager";
        }
    }

    void Application::cleanApplication() {
        utl::MessagePump::destroy();

        res_mgr_->destroy();

        ilf_->destroy();
        ilf_.reset();

        imm_->destroy();
        imm_.reset();

        gdm_->destroy();
        gdm_.reset();

        cleanPlatform();
    }

    void Application::run() {
        utl::MessagePump::run();
    }

    // static
    void Application::setVSync(bool enable){
        vsync_enabled_ = enable;
    }

    // static
    bool Application::isVSyncEnabled(){
        return vsync_enabled_;
    }

    // static
    LcImageFactory* Application::getImageLocFactory() {
        return instance_->ilf_.get();
    }

    // static
    InputMethodManager* Application::getInputMethodManager() {
        return instance_->imm_.get();
    }

    // static
    ResourceManager* Application::getResourceManager() {
        return instance_->res_mgr_.get();
    }

    // static
    GraphicDeviceManager* Application::getGraphicDeviceManager() {
        return instance_->gdm_.get();
    }

    // static
    int Application::getViewID() {
        ++view_uid_;
        return view_uid_;
    }

    // static
    const Application::Options& Application::getOptions() {
        return instance_->options_;
    }

    // static
    float Application::dp2px(int dp) {
        if (instance_->options_.is_auto_dpi_scale) {
            return float(dp);
        }

        float sx, sy;
        Display::primary()->getUserScale(&sx, &sy);
        return sx * dp;
    }

    // static
    float Application::dp2px(float dp) {
        if (instance_->options_.is_auto_dpi_scale) {
            return dp;
        }

        float sx, sy;
        Display::primary()->getUserScale(&sx, &sy);
        return sy * dp;
    }

    // static
    int Application::dp2pxi(int dp) {
        if (instance_->options_.is_auto_dpi_scale) {
            return dp;
        }

        float sx, sy;
        Display::primary()->getUserScale(&sx, &sy);
        return static_cast<int>(sx * dp);
    }

    // static
    int Application::dp2pxi(float dp) {
        if (instance_->options_.is_auto_dpi_scale) {
            return int(dp);
        }

        float sx, sy;
        Display::primary()->getUserScale(&sx, &sy);
        return static_cast<int>(sy * dp);
    }

}
