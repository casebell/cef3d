//----------------------------------------------------------------------------
// Cef3D
// Copyright (C) 2017 arkenthera
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// https://github.com/arkenthera/cef3d
// Cef3DBrowserWindow.h
// Date: 26.04.2017
//---------------------------------------------------------------------------

#include "Cef3DPCH.h"

namespace Cef3D
{
	RootWindowWin::RootWindowWin()
		: delegate_(NULL),
		with_osr_(false),
		is_popup_(false),
		start_rect_(),
		initialized_(false),
		window_destroyed_(false),
		browser_destroyed_(false)
	{
	}

	RootWindowWin::~RootWindowWin() {
		REQUIRE_MAIN_THREAD();

		// The window and browser should already have been destroyed.
		DCHECK(window_destroyed_);
		DCHECK(browser_destroyed_);
	}

	void RootWindowWin::Init(RootWindow::Delegate* delegate,
		bool with_osr,
		const CefRect& bounds,
		const CefBrowserSettings& settings,
		const std::string& url) {
		DCHECK(delegate);
		DCHECK(!initialized_);

		delegate_ = delegate;
		with_osr_ = with_osr;

		start_rect_.left = bounds.x;
		start_rect_.top = bounds.y;
		start_rect_.right = bounds.x + bounds.width;
		start_rect_.bottom = bounds.y + bounds.height;

		CreateBrowserWindow(url);

		initialized_ = true;

		// Create the native root window on the main thread.
		if (CURRENTLY_ON_MAIN_THREAD()) {
			CreateRootWindow(settings);
		}
		else {
			MAIN_POST_CLOSURE(
				base::Bind(&RootWindowWin::CreateRootWindow, this, settings));
		}
	}

	void RootWindowWin::InitAsPopup(RootWindow::Delegate* delegate,
		bool with_osr,
		const CefPopupFeatures& popupFeatures,
		CefWindowInfo& windowInfo,
		CefRefPtr<CefClient>& client,
		CefBrowserSettings& settings) {
		DCHECK(delegate);
		DCHECK(!initialized_);

		delegate_ = delegate;
		with_osr_ = with_osr;
		is_popup_ = true;

		if (popupFeatures.xSet)
			start_rect_.left = popupFeatures.x;
		if (popupFeatures.ySet)
			start_rect_.top = popupFeatures.y;
		if (popupFeatures.widthSet)
			start_rect_.right = start_rect_.left + popupFeatures.width;
		if (popupFeatures.heightSet)
			start_rect_.bottom = start_rect_.top + popupFeatures.height;

		CreateBrowserWindow(std::string());

		initialized_ = true;

		// The new popup is initially parented to a temporary window. The native root
		// window will be created after the browser is created and the popup window
		// will be re-parented to it at that time.
		browser_window_->GetPopupConfig(Cef3DTempWindow::GetWindowHandle(),
			windowInfo, client, settings);
	}

	void RootWindowWin::Show(ShowMode mode) {
		REQUIRE_MAIN_THREAD();
	}

	void RootWindowWin::Hide() {
		REQUIRE_MAIN_THREAD();


	}

	void RootWindowWin::SetBounds(int x, int y, size_t width, size_t height) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::Close(bool force) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::SetDeviceScaleFactor(float device_scale_factor) {
		REQUIRE_MAIN_THREAD();

		if (browser_window_)
			browser_window_->SetDeviceScaleFactor(device_scale_factor);
	}

	float RootWindowWin::GetDeviceScaleFactor() const {
		REQUIRE_MAIN_THREAD();

		if (browser_window_)
			return browser_window_->GetDeviceScaleFactor();
		return 1.0f;
	}

	CefRefPtr<CefBrowser> RootWindowWin::GetBrowser() const {
		REQUIRE_MAIN_THREAD();

		if (browser_window_)
			return browser_window_->GetBrowser();
		return NULL;
	}

	WindowHandle RootWindowWin::GetWindowHandle() const {
		REQUIRE_MAIN_THREAD();
		return 0;
	}

	void RootWindowWin::CreateBrowserWindow(const std::string& startup_url)
	{
		DCHECK(with_osr_);

		if (with_osr_) {
			Cef3DOSRSettings settings;
			GMainContext->PopulateOsrSettings(&settings);
			browser_window_.reset(new BrowserWindowOsrWin(this, startup_url, settings));
		}
	}

	void RootWindowWin::CreateRootWindow(const CefBrowserSettings& settings) {
		REQUIRE_MAIN_THREAD();
		

		if (!is_popup_) {
			// Create the browser window.
			CefRect cef_rect(0,0,800,600);
			browser_window_->CreateBrowser(NULL, cef_rect, settings,
				delegate_->GetRequestContext(this));
		}
		else {
			// With popups we already have a browser window. Parent the browser window
			// to the root window and show it in the correct location.
			browser_window_->ShowPopup(NULL,0,0,800,600);
		}

		// Show this window.
		Show(ShowNormal);
	}

	bool RootWindowWin::OnClose() {
		if (browser_window_ && !browser_window_->IsClosing()) {
			CefRefPtr<CefBrowser> browser = GetBrowser();
			if (browser) {
				// Notify the browser window that we would like to close it. This
				// will result in a call to ClientHandler::DoClose() if the
				// JavaScript 'onbeforeunload' event handler allows it.
				browser->GetHost()->CloseBrowser(false);

				// Cancel the close.
				return true;
			}
		}

		// Allow the close.
		return false;
	}


	void RootWindowWin::OnFocus() {
		if (browser_window_)
			browser_window_->SetFocus(true);
	}

	void RootWindowWin::OnSize(bool minimized) {
	}

	void RootWindowWin::OnMove() {
		// Notify the browser of move events so that popup windows are displayed
		// in the correct location and dismissed when the window moves.
		CefRefPtr<CefBrowser> browser = GetBrowser();
		if (browser)
			browser->GetHost()->NotifyMoveOrResizeStarted();
	}

	void RootWindowWin::OnDestroyed() {
		window_destroyed_ = true;
		NotifyDestroyedIfDone();
	}

	void RootWindowWin::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
		REQUIRE_MAIN_THREAD();

		if (is_popup_) {
			// For popup browsers create the root window once the browser has been
			// created.
			CreateRootWindow(CefBrowserSettings());
		}
		else {
			// Make sure the browser is sized correctly.
			OnSize(false);
		}
	}

	void RootWindowWin::OnBrowserWindowDestroyed() {
		REQUIRE_MAIN_THREAD();

		browser_window_.reset();

		if (!window_destroyed_) {
			// The browser was destroyed first. This could be due to the use of
			// off-screen rendering or execution of JavaScript window.close().
			// Close the RootWindow.
			Close(true);
		}

		browser_destroyed_ = true;
		NotifyDestroyedIfDone();
	}

	void RootWindowWin::OnSetAddress(const std::string& url) {
		REQUIRE_MAIN_THREAD();
	}

	void RootWindowWin::OnSetTitle(const std::string& title) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::OnSetFullscreen(bool fullscreen) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::OnSetLoadingState(bool isLoading,
		bool canGoBack,
		bool canGoForward) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::OnSetDraggableRegions(
		const std::vector<CefDraggableRegion>& regions) {
		REQUIRE_MAIN_THREAD();

	}

	void RootWindowWin::NotifyDestroyedIfDone() {
		// Notify once both the window and the browser have been destroyed.
		if (window_destroyed_ && browser_destroyed_)
			delegate_->OnRootWindowDestroyed(this);
	}
}