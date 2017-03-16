#include "../include/tjbrowser.h"
using namespace tj::browser;

extern "C" { 
	__declspec(dllexport) std::vector< ref<Plugin> >* GetPlugins() {
		std::vector<ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();
		plugins->push_back(GC::Hold(new BrowserPlugin()));
		return plugins;
	}
}