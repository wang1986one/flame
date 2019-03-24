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
#include "tag.h"
#include "pic.h"

#include <flame/file.h>
#include <flame/string.h>

using namespace flame;

void delete_tag(Tag *t)
{
	for (auto &p : t->pics)
	{
		p->remove_tag(t);

		auto prev_filename = p->filename;
		p->make_filename_from_tags();
		std::filesystem::rename(prev_filename, p->filename);
	}
}

void Tag::remove_pic(Pic *p)
{
	for (auto it = pics.begin(); it != pics.end(); it++)
	{
		if (*it == p)
		{
			pics.erase(it);
			break;
		}
	}
}

Tag *tag_ungrouped;
std::vector<std::unique_ptr<Tag>> tags;

void sort_tags()
{
	std::sort(tags.begin(), tags.end(), [](const std::unique_ptr<Tag> &a, const std::unique_ptr<Tag> &b) {
		return a->name < b->name;
	});
}

Tag *get_tag(int id)
{
	for (auto &t : tags)
	{
		if (t->id == id)
			return t.get();
	}
	return nullptr;
}

Tag *find_tag(const std::wstring &name)
{
	for (auto &t : tags)
	{
		if (t->name == name)
			return t.get();
	}
	return nullptr;
}

Tag *add_tag(const std::wstring &name, int id)
{
	auto tag = new Tag;
	tag->id = id;
	tag->name = name;
	tag->use = false;
	tags.emplace_back(tag);

	sort_tags();

	return tag;
}

Tag *add_tag(const std::wstring &name)
{
	for (auto try_id = 0; try_id < tags.size() + 1; try_id++)
	{
		auto ok = true;
		for (auto &t : tags)
		{
			if (t->id == try_id)
			{
				ok = false;
				break;
			}
		}
		if (ok)
		{
			auto tag = new Tag;
			tag->id = try_id;
			tag->name = name;
			tag->use = false;
			tags.emplace_back(tag);

			sort_tags();

			return tag;
		}
	}
	return nullptr;
}

void load_tags()
{
	std::ifstream tags_file(work_dir + L"/TAGS");
	if (!tags_file.good())
	{
		std::ofstream create(work_dir + L"/TAGS");
		create.close();
	}
	else
	{
		while (!tags_file.eof())
		{
			std::string line;
			std::getline(tags_file, line);
			if (line.empty())
				break;
			auto tag = new Tag;
			tag->id = std::stoi(line);
			std::getline(tags_file, line);
			tag->name = s2w(line);
			tag->use = false;
			tags.emplace_back(tag);
		}
		tags_file.close();
	}
}

void save_tags()
{
	std::ofstream tags_file(work_dir + L"/TAGS");
	if (tags_file.good())
	{
		for (auto &t : tags)
		{
			tags_file << t->id;
			tags_file << std::endl;
			tags_file << w2s(t->name);
			tags_file << std::endl;
		}
		tags_file.close();
	}
}

std::vector<std::wstring> guess_tags_from_filename(const std::wstring &filename)
{
	std::vector<std::wstring> tags;

	std::filesystem::path curr_path(filename);
	std::filesystem::path target_path(work_dir);

	while (true)
	{
		auto parent_path = curr_path.parent_path();
		if (parent_path == target_path)
			break;
		auto tag_name = parent_path.filename().generic_wstring();
		auto sp = string_split(tag_name);
		for (auto &s : sp)
		{
			if (s[0] == L' ')
				continue;

			auto idx = tags.size();
			tags.emplace_back(s);
		}
		curr_path = parent_path;
	}

	return tags;
}