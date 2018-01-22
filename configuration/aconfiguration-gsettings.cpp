/*
    Aeskulap Configuration - persistent configuration interface library
    Copyright (C) 2005  Alexander Pipelka

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Alexander Pipelka
*/

#include <giomm.h>
#include <glibmm.h>
#include <cstdio>
#include <iostream>
#include "aconfiguration.h"

namespace Aeskulap {
	
using Gio::Settings; 
using PSettings = Glib::RefPtr<Settings>; 

struct ConfigurationImpl {

        ConfigurationImpl();

	PSettings settings_prefs;
	PSettings settings_presets;
	
        bool has_modality(const Glib::ustring& modality);
};

ConfigurationImpl::ConfigurationImpl():
        settings_prefs(Settings::create("org.gnu.aeskulap")),
        settings_presets(Settings::create("org.gnu.aeskulap.presets"))
{

}

bool ConfigurationImpl::has_modality(const Glib::ustring& modality)
{
        auto children = settings_presets->list_children();
        bool has_modality = false;
        for (auto c = children.begin(); !has_modality && c != children.end(); ++c)
                has_modality = (*c == modality);
        return has_modality;
}

Configuration::Configuration() {
        std::cout << "Gio::Settings init" << std::endl;
        Gio::init();

        impl = new ConfigurationImpl();

        if (!impl->has_modality("CT"))
                add_default_presets_ct();
}

std::string Configuration::get_local_aet() {

        Glib::ustring local_aet = impl->settings_prefs->get_string("local_aet");

        if(local_aet.empty()) {
                local_aet = "AESKULAP";
                set_local_aet(local_aet);
        }

        return std::string(local_aet.c_str());
}

void Configuration::set_local_aet(const std::string& aet) {
        impl->settings_prefs->set_string("local_aet", aet);
}

unsigned int Configuration::get_local_port() {

        gint local_port = impl->settings_prefs->get_int("local_port");

        if(local_port <= 0) {
                local_port = 6000;
                set_local_port(local_port);
        }

        return (unsigned int)local_port;
}

void Configuration::set_local_port(unsigned int port) {
        if(port <= 0) {
                port = 6000;
        }
        impl->settings_prefs->set_int("local_port", (gint)port);
}

std::string Configuration::get_encoding() {
        Glib::ustring charset = impl->settings_prefs->get_string("characterset");

        if(charset.empty()) {
                charset = "ISO_IR 100";
                set_encoding(charset);
        }

        return charset.c_str();
}

void Configuration::set_encoding(const std::string& encoding) {
        impl->settings_prefs->set_string("characterset", encoding);
}

std::vector<int> convert_to_int_array(const std::vector<Glib::ustring>& in) {
	std::vector<int> result(in.size()); 
	
	transform(in.begin(), in.end(), result.begin(), [](const Glib::ustring& x) {
		std::istringstream s(x.c_str()); 
		int o; 
		s >> o; 
		return o; 
	}); 
	return result; 
}

std::vector<bool> convert_to_bool_array(const std::vector<Glib::ustring>& in) {
	std::vector<bool> result(in.size()); 
	
	transform(in.begin(), in.end(), result.begin(), [](const Glib::ustring& x) {
		return x == "true"; 
	}); 
	return result; 
}


Configuration::ServerList* Configuration::get_serverlist() {
        Configuration::ServerList* list = new Configuration::ServerList;

        std::vector<Glib::ustring> aet_list = impl->settings_prefs->get_string_array("server_aet");
        std::vector<int> port_list = convert_to_int_array(impl->settings_prefs->get_string_array("server_port"));
        std::vector<Glib::ustring> hostname_list = impl->settings_prefs->get_string_array("server_hostname");
        std::vector<Glib::ustring> description_list = impl->settings_prefs->get_string_array("server_description");
        std::vector<Glib::ustring> group_list = impl->settings_prefs->get_string_array("server_group");
        std::vector<bool> lossy_list = convert_to_bool_array(impl->settings_prefs->get_string_array("server_lossy"));
        std::vector<bool> relational_list = convert_to_bool_array(impl->settings_prefs->get_string_array("server_relational"));

        auto a = aet_list.begin();
        auto p = port_list.begin();
        auto h = hostname_list.begin();
        auto d = description_list.begin();
        auto g = group_list.begin();
        auto l = lossy_list.begin();
        auto r = relational_list.begin();

        for(; h != hostname_list.end() && a != aet_list.end() && p != port_list.end(); a++, p++, h++) {

                std::string servername;
                if(d != description_list.end()) {
                        servername = *d;
                        d++;
                }
                else {
                        char buffer[50];
                        snprintf(buffer, sizeof(buffer), "Server%li", list->size()+1);
                        servername = buffer;
                }

                ServerData& s = (*list)[servername];
                s.m_aet = *a;
		s.m_port = *p;
                s.m_hostname = *h;
                s.m_name = servername;
                s.m_lossy = false;
                s.m_relational = false;

                if(g != group_list.end()) {
                        s.m_group = *g;
                        g++;
                }
                
                if ( l != lossy_list.end()) {
			s.m_lossy = *l; 
			++l; 
		}

                if(r != relational_list.end()) {
                        s.m_relational = *r;
                        r++;
                }

        }

        return list;
}

void Configuration::set_serverlist(std::vector<ServerData>& list) {

        std::vector<Glib::ustring> aet_list;
        std::vector<Glib::ustring> hostname_list;
        std::vector<Glib::ustring> port_list;
        std::vector<Glib::ustring> description_list;
        std::vector<Glib::ustring> group_list;
        std::vector<Glib::ustring> lossy_list;
        std::vector<Glib::ustring> relational_list;

        std::vector<ServerData>::iterator i;
        for(i = list.begin(); i != list.end(); i++) {
                aet_list. push_back(i->m_aet);
                hostname_list.push_back(i->m_hostname);
                port_list.push_back( Glib::ustring::compose("%d", i->m_port));
		description_list.push_back(i->m_name);
                group_list.push_back(i->m_group);
                lossy_list.push_back(i->m_lossy ? "true": "false");
                relational_list.push_back(i->m_relational ? "true": "false");
        }

        impl->settings_prefs->set_string_array("server_aet", aet_list);
        impl->settings_prefs->set_string_array("server_hostname", hostname_list);
        impl->settings_prefs->set_string_array("server_port", port_list);
        impl->settings_prefs->set_string_array("server_description", description_list);
        impl->settings_prefs->set_string_array("server_group", group_list);
        impl->settings_prefs->set_string_array("server_lossy", lossy_list);
        impl->settings_prefs->set_string_array("server_relational", relational_list);
}

bool Configuration::get_windowlevel(const Glib::ustring&  modality, const Glib::ustring& desc, WindowLevel& w) {

	auto modality_settings = impl->settings_presets->get_child(modality); 
	if (!modality_settings) 
		return false; 
	
	auto tissue_settings = modality_settings->get_child(desc); 
	if (!tissue_settings) 
		return false; 

        w.modality = modality;
        w.description = desc;
        w.center = tissue_settings->get_int("center");
        w.width = tissue_settings->get_int("width");

        return true;
}

bool Configuration::get_windowlevel_list(const Glib::ustring& modality, WindowLevelList& list) {
	
        if(modality.empty()) {
                return false;
        }

        auto modality_settings = impl->settings_presets->get_child(modality); 
	if (!modality_settings) 
		return false; 

        auto dirs = modality_settings->list_children();
	if (dirs.empty()) 
		return false; 

        for(unsigned int i=0; i<dirs.size(); i++) {
                WindowLevel w;
		w.modality = modality; 
		w.description = dirs[i]; 
		auto tissue_settings = modality_settings->get_child(w.description); 
		w.center = tissue_settings->get_int("center");
		w.width = tissue_settings->get_int("width");
        }
        return true;
}

bool Configuration::set_windowlevel(const WindowLevel& w) {
	
        auto modality_settings = impl->settings_presets->get_child(w.modality); 
	g_assert(modality_settings); 
	g_return_val_if_fail(modality_settings, false); 
		
	auto tissue_settings = modality_settings->get_child(w.description); 
	
	if (!tissue_settings) {
		// create settings 
		std::string pp = modality_settings->property_path(); 
		pp.append("/").append(w.description);
		
		tissue_settings = Settings::create("org.gnu.aeskulap.presets.modality.tissue", pp); 
	}
	
	tissue_settings->set_int("center", w.center);
	tissue_settings->set_int("width", w.width);

        return true;
}

bool Configuration::set_windowlevel_list(const Glib::ustring& modality, WindowLevelList& list) {

	auto modality_settings = impl->settings_presets->get_child(modality); 
	if (!modality_settings) {
		std::string pp = impl->settings_presets->property_path(); 
		pp.append("/").append(modality);
		
		modality_settings = Settings::create("org.gnu.aeskulap.presets.modality", pp);
	}
	
        for(auto i = list.begin(); i != list.end(); i++) {
                i->second.modality = modality;
                set_windowlevel(i->second);
        }

        return true;
}

bool Configuration::unset_windowlevels(const Glib::ustring& modality) {
	auto modality_settings = impl->settings_presets->get_child(modality); 
	if (modality_settings) {
		auto tissues = modality_settings->list_keys();
		for (auto k: tissues)
			modality_settings->reset(k); 
	}
	return true; 
}

} // namespace Aeskulap
