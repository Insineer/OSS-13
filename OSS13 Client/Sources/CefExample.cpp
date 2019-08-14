#include "CefExample.h"

#include "include/cef_client.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_base.h"
#include "include/base/cef_lock.h"
//#include "include/base/cef_ref_counted.h"
//#include <include/cef_browser.h>
#include <include/views/cef_browser_view.h>
#include <include/base/cef_logging.h>
#include <include/views/cef_window.h>
#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
//#include <include/views/cef_window.h>
//#include <include/internal/cef_types_wrappers.h>
//#include <include/internal/cef_ptr.h>
//#include <include/base/cef_scoped_ptr.h>
//#include <include/internal/cef_win.h>

#include <list>

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
public:
	explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
		: browser_view_(browser_view) {}

	void OnWindowCreated(CefRefPtr<CefWindow> window) {
		// Add the browser view and show the window.
		window->AddChildView(browser_view_);
		window->Show();

		// Give keyboard focus to the browser view.
		browser_view_->RequestFocus();
	}

	void OnWindowDestroyed(CefRefPtr<CefWindow> window) {
		browser_view_ = NULL;
	}

	bool CanClose(CefRefPtr<CefWindow> window) {
		// Allow the window to close if the browser says it's OK.
		CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
		if (browser)
			return browser->GetHost()->TryCloseBrowser();
		return true;
	}

	CefSize GetPreferredSize(CefRefPtr<CefView> view) {
		return CefSize(800, 600);
	}

private:
	CefRefPtr<CefBrowserView> browser_view_;

	IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
	DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

}  // namespace

class SimpleHandler;

namespace {

	SimpleHandler* g_instance = NULL;

}  // namespace

class SimpleHandler : public CefClient,
	public CefDisplayHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler {
public:
	explicit SimpleHandler(bool use_views): use_views_(use_views), is_closing_(false) {
		DCHECK(!g_instance);
		g_instance = this;
	}
	~SimpleHandler() {
		g_instance = NULL;
	}

	// Provide access to the single global instance of this object.
	static SimpleHandler* GetInstance() {
		return g_instance;
	}

	// CefClient methods:
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() { return this; }
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() { return this; }

	// CefDisplayHandler methods:
	void OnTitleChange(CefRefPtr<CefBrowser> browser,
		const CefString& title) {
		CEF_REQUIRE_UI_THREAD();

		if (use_views_) {
			// Set the title of the window using the Views framework.
			CefRefPtr<CefBrowserView> browser_view =
				CefBrowserView::GetForBrowser(browser);
			if (browser_view) {
				CefRefPtr<CefWindow> window = browser_view->GetWindow();
				if (window)
					window->SetTitle(title);
			}
		}
		else {
			// Set the title of the window using platform APIs.
			PlatformTitleChange(browser, title);
		}
	}

	// CefLifeSpanHandler methods:
	void OnAfterCreated(CefRefPtr<CefBrowser> browser) {
		CEF_REQUIRE_UI_THREAD();

		// Add to the list of existing browsers.
		browser_list_.push_back(browser);
	}

	CefRefPtr<CefBrowser> GetBrowser() {
		return browser_list_.back();
	}

	bool DoClose(CefRefPtr<CefBrowser> browser) {
		CEF_REQUIRE_UI_THREAD();

		// Closing the main window requires special handling. See the DoClose()
		// documentation in the CEF header for a detailed destription of this
		// process.
		if (browser_list_.size() == 1) {
			// Set a flag to indicate that the window close should be allowed.
			is_closing_ = true;
		}

		// Allow the close. For windowed browsers this will result in the OS close
		// event being sent.
		return false;
	}

	void OnBeforeClose(CefRefPtr<CefBrowser> browser) {
		CEF_REQUIRE_UI_THREAD();

		// Remove from the list of existing browsers.
		BrowserList::iterator bit = browser_list_.begin();
		for (; bit != browser_list_.end(); ++bit) {
			if ((*bit)->IsSame(browser)) {
				browser_list_.erase(bit);
				break;
			}
		}

		if (browser_list_.empty()) {
			// All browser windows have closed. Quit the application message loop.
			CefQuitMessageLoop();
		}
	}

	void OnLoadError(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		ErrorCode errorCode,
		const CefString& errorText,
		const CefString& failedUrl) {
		CEF_REQUIRE_UI_THREAD();

		// Don't display an error for downloaded files.
		if (errorCode == ERR_ABORTED)
			return;

		// Display a load error message.
		std::stringstream ss;
		ss << "<html><body bgcolor=\"white\">"
			"<h2>Failed to load URL "
			<< std::string(failedUrl) << " with error " << std::string(errorText)
			<< " (" << errorCode << ").</h2></body></html>";
		frame->LoadString(ss.str(), failedUrl);
	}

	void CloseAllBrowsers(bool force_close) {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute on the UI thread.
			CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this,
				force_close));
			return;
		}

		if (browser_list_.empty())
			return;

		BrowserList::const_iterator it = browser_list_.begin();
		for (; it != browser_list_.end(); ++it)
			(*it)->GetHost()->CloseBrowser(force_close);
	}

	bool IsClosing() const { return is_closing_; }

private:
	// Platform-specific implementation.
	void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
		const CefString& title) {
		CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
		SetWindowText(hwnd, std::wstring(title).c_str());
	}


	// True if the application is using the Views framework.
	const bool use_views_;

	// List of existing browser windows. Only accessed on the CEF UI thread.
	typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
	BrowserList browser_list_;

	bool is_closing_;

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(SimpleHandler);
};

class SimpleApp : public CefApp, public CefBrowserProcessHandler {
public:
	SimpleApp() {};

	// CefApp methods:
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() {
		return this;
	}

	// CefBrowserProcessHandler methods:
	virtual void OnContextInitialized() {
		CEF_REQUIRE_UI_THREAD();

		CefRefPtr<CefCommandLine> command_line =
			CefCommandLine::GetGlobalCommandLine();

		const bool use_views = command_line->HasSwitch("use-views");

		// SimpleHandler implements browser-level callbacks.
		CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

		// Specify CEF browser settings here.
		CefBrowserSettings browser_settings;

		std::string url;

		// Check if a "--url=" value was provided via the command-line. If so, use
		// that instead of the default URL.
		url = command_line->GetSwitchValue("url");
		if (url.empty())
			url = "http://www.google.com";

		if (use_views) {
			// Create the BrowserView.
			CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
				handler, url, browser_settings, NULL, NULL, NULL);

			// Create the Window. It will show itself after creation.
			CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
		}
		else {
			// Information used when creating the native window.
			CefWindowInfo window_info;

			// On Windows we need to specify certain flags that will be passed to
			// CreateWindowEx().
			window_info.SetAsPopup(NULL, "cefsimple");

			// Create the first browser window.
			CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
				NULL, NULL);

			/*CefRefPtr<CefBrowser> browser = GetCurrentContext()->GetBrowser();
			browser->GetMainFrame()->LoadStringW("<p>This is a paragraph.</p>", "");*/
		}

		//handler->GetBrowser()->GetMainFrame()->LoadStringW("<p>This is a paragraph.</p>", "");
	}

private:
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(SimpleApp);
};

void CefCheck()
{
	CefMainArgs main_args;
	// CEF applications have multiple sub-processes (render, plugin, GPU, etc)
	// that share the same executable. This function checks the command-line and,
	// if this is a sub-process, executes the appropriate logic.
	int exit_code = CefExecuteProcess(main_args, NULL, NULL);
	if (exit_code >= 0) {
		return;
	}

	CefSettings settings;

	CefRefPtr<SimpleApp> app(new SimpleApp);

	// Initialize CEF.
	CefInitialize(main_args, settings, app.get(), NULL);

	// Run the CEF message loop. This will block until CefQuitMessageLoop() is
	// called.
	CefRunMessageLoop();

	// Shut down CEF.
	CefShutdown();

	//CefBrowserSettings browser_settings;
	//CefWindowInfo window_info;
	//window_info.SetAsPopup(NULL, "cefsimple");
	//auto url = "http://www.google.com";
	////CefBrowserHost::CreateBrowser(window_info, NULL, url, browser_settings, NULL, NULL);
	//CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(window_info, NULL, url, browser_settings, NULL, NULL);
	//auto handle = browser->GetHost();//->GetWindowHandle();
	////SetWindowPos(handle, NULL, 100, 100, 500, 500, SWP_NOZORDER);
}