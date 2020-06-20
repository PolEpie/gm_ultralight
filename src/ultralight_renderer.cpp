#include <Ultralight/Ultralight.h>
#include <chrono>   // std::chrono::milliseconds
#include <thread>   // std::this_thread::sleep_for
#include <string>   // std::string
#include <iostream> // std::cout
#include <cstring>  // memcpy
#include <vector>   // std::vector
#include "shoom/shm.h"

using namespace ultralight;

#define URLLEN 512

RefPtr<Renderer> renderer;

class IView :public LoadListener {
	Shm* SHMwidth;
	Shm* SHMheight;
public:
	Shm* SHMurl;
	Shm* SHMisloaded;
	Shm* SHMsync;
	Shm* image;
	uint32_t width = 0;
	uint32_t height = 0;
	char* url;
	RefPtr<View> view;
	bool loaded = false;
	uint8_t vid;

	IView(uint8_t id) {
		vid = id;
		SHMwidth = new Shm{ std::string("ul_o_width_").append(std::to_string(id)), 64 };
		SHMwidth->Open();
		width = std::stoi((char*)SHMwidth->Data());

		SHMheight = new Shm{ std::string("ul_o_height_").append(std::to_string(id)), 64 };
		SHMheight->Open();
		height = std::stoi((char*)SHMheight->Data());

		SHMurl = new Shm{ std::string("ul_o_url_").append(std::to_string(id)), URLLEN };
		url = "placeholder";

		image = new Shm{ std::string("ul_i_image_").append(std::to_string(id)), 1 + (width * height * 4) };
		image->Create();

		SHMisloaded = new Shm{ std::string("ul_i_isloaded_").append(std::to_string(id)), 16 };
		SHMisloaded->Create();

		SHMsync = new Shm{ std::string("ul_i_sync_").append(std::to_string(id)), 16 };
		SHMsync->Create();

		view = renderer->CreateView(width * 4, height * 4, false); // https://github.com/ultralight-ux/Ultralight/issues/257#issuecomment-636330995
		view->Resize(width, height);
		view->set_load_listener(this);
	}
	void thonk() {
		SHMurl->Open();
		if (std::string((char*)SHMurl->Data()) != "" && std::string((char*)SHMurl->Data()) != url) {
			loaded = false;
			SHMisloaded->Data()[0] = 0;
			url = (char*)SHMurl->Data();
			view->LoadURL(url);
		}
		if (loaded && view->is_bitmap_dirty()) {
			memcpy(image->Data(), view->bitmap()->LockPixels(), width * height * 4);
			view->bitmap()->UnlockPixels();
			SHMisloaded->Data()[0] = 1;
			SHMsync->Data()[0] = SHMsync->Data()[0] > 100 ? 0 : SHMsync->Data()[0] + 1;
		}
	}
	~IView() {
		view = nullptr;
		delete SHMwidth;
		delete SHMheight;
		delete SHMurl;
		delete SHMisloaded;
		delete SHMsync;
		delete image;
	}
	void OnFinishLoading(View* caller) {
		loaded = true;
	}
};

int main() {
	std::cout << "starting renderer" << std::endl;
	std::vector<IView*> views;
	Shm ul_o_rpc{ "ul_o_rpc", 128 };
	Shm ul_i_rpc{ "ul_i_rpc", 128 };
	Shm ul_o_createview{ "ul_o_createview", 200 };
	ul_i_rpc.Create();
	Config config;
	config.device_scale_hint = 1.0;
	config.font_family_standard = "Arial";
	Platform::instance().set_config(config);
	renderer = Renderer::Create();
	std::cout << "while" << std::endl;
	while (true) {
		std::cout << "this_thread::sleep_for" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout << "cout" << std::endl;
		std::cout << "opening rpc" << std::endl;
		ul_o_rpc.Open();
		if (ul_o_rpc.Data() == nullptr) continue;
		if (ul_o_rpc.Data()[0] != 0) { std::cout << "ul_o_rpc.Data()[0]!=0" << std::endl; break; }
		ul_o_createview.Open();
		if (ul_o_createview.Data() != nullptr) {
			for (uint8_t i = 0; i < 200; i++) // sizeof(ul_o_createview.Data()) / sizeof(uint8_t)
			{
				uint8_t id = ul_o_createview.Data()[i];
				if (id == 1 && i > views.size()) {
					IView* view = new IView(id);
					views.push_back(view);
				}
			}
		}
		for (uint32_t i = 0;i < 1000;i++) {
			renderer->Update();
		}
		renderer->Render();
		for each (IView * view in views)
		{
			view->thonk();
		}
	}
	std::cout << "closing..." << std::endl;
	renderer = nullptr;
	return 0;
}