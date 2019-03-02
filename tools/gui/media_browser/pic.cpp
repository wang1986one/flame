// MIT License
// 
// Copyright (c) 2018 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "share.h"
#include "pic.h"
#include "tag.h"

#include <flame/file.h>

using namespace flame;

Pic::Pic()
{
	img_thumbnail = nullptr;
	w_img = nullptr;
}

bool Pic::has_tag(Tag *tag)
{
	for (auto &t : tags)
	{
		if (t == tag)
			return true;
	}
	return false;
}

void Pic::remove_tag(Tag *tag)
{
	for (auto it = tags.begin(); it != tags.end(); it++)
	{
		if (*it == tag)
		{
			tags.erase(it);
			break;
		}
	}
}

void Pic::get_tags_from_filename()
{
	auto stem = std::filesystem::path(filename).stem().generic_wstring();
	if (stem[0] == L'~')
	{
		auto tags_id_sp = string_regex_split(stem, std::wstring(L"~T([0-9]+)"), 1);
		for (auto &s : tags_id_sp)
		{
			auto tag_id = std::stoi(s);
			auto tag = get_tag(tag_id);
			if (!tag)
			{
				tag = add_tag(L"dummy" + s, tag_id);
				save_tags();
			}
			if (!has_tag(tag))
			{
				tags.push_back(tag);
				tag->pics.push_back(this);
			}
		}
	}
}

void Pic::make_filename_from_tags()
{
	std::filesystem::path path(filename);
	auto stem = path.stem().generic_wstring();
	if (stem[0] == L'~')
		stem = string_regex_split(stem, std::wstring(L"(~N[\\w]+)"), 1)[0];
	else
	{
		auto hash = H(w2s(filename).c_str());
		wchar_t buf[20];
		swprintf(buf, L"%X", hash);
		stem = L"~N";
		stem += buf;
	}
	for (auto &t : tags)
	{
		stem += L"~T";
		stem += std::to_wstring(t->id);
	}
	filename = path.parent_path().generic_wstring() + L"/" + stem + path.extension().generic_wstring();
}

void delete_pic(Pic *p)
{
	for (auto &t : p->tags)
		t->remove_pic(p);
}

std::vector<std::unique_ptr<Pic>> pics;

void load_pics()
{
	for (std::filesystem::recursive_directory_iterator end, it(work_dir); it != end; it++)
	{
		if (!std::filesystem::is_directory(it->status()))
		{
			auto ext = it->path().extension();
			if (ext == L".jpg" ||
				ext == L".jpeg" ||
				ext == L".png" ||
				ext == L".JPG" ||
				ext == L".PNG")
			{
				auto pic = new Pic;
				pic->filename = it->path().generic_wstring();
				pic->get_tags_from_filename();
				pics.emplace_back(pic);
			}
		}
	}
}