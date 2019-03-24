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

#include <flame/foundation/foundation.h>
#include <flame/foundation/serialize.h>

using namespace flame;

int main(int argc, char **args)
{
	if (argc == 1)
		return 0;

	std::vector<std::wstring> pdb_dirs;
	std::vector<std::wstring> pdb_filenames;
	std::wstring output_filename(L"typeinfo.xml");

	for (auto i = 1; i < argc; i++)
	{
		if (args[i][0] != '-')
			pdb_dirs.push_back(s2w(args[i]));
		else
		{
			if (args[i][1] == 'o')
				output_filename = s2w(&args[i][2]);
		}
	}

	std::string cmd_prefix("\"");
	cmd_prefix += VS_LOCATION;
	cmd_prefix += "/Common7/Tools/VsDevCmd.bat\" & dumpbin /DEPENDENTS ";

	for (auto& d : pdb_dirs)
	{
		for (std::filesystem::directory_iterator end, it(d); it != end; it++)
		{
			if (!std::filesystem::is_directory(it->status()))
			{
				if (it->path().filename() == L"flame_foundation.dll")
				{
					pdb_filenames.push_back(it->path().wstring());
					continue;
				}

				auto ext = it->path().extension();
				if (ext == L".dll" || ext == L".exe")
				{
					auto dependancies = get_module_dependancies(it->path().wstring().c_str());
					for (auto i = 0; i < dependancies.size; i++)
					{
						if (dependancies[i] == "flame_foundation.dll")
						{
							pdb_filenames.push_back(it->path().wstring());
							break;
						}
					}
				}
			}
		}
	}

	typeinfo_collect_init();
	typeinfo_collect(pdb_filenames);
	typeinfo_save(output_filename);
	typeinfo_clear();

	return 0;
}