#include "app.h"

cConsole::cConsole() :
	Component("cConsole")
{
	auto e_page = ui::e_begin_docker_window(L"Console").second;
	{
		e_page->get_component(cElement)->inner_padding_ = Vec4f(8.f);
		auto c_layout = ui::c_layout(LayoutVertical);
		c_layout->item_padding = 4.f;
		c_layout->width_fit_children = false;
		c_layout->height_fit_children = false;

		e_page->add_component(this);
	}

	ui::e_begin_scroll_view1(ScrollbarVertical, Vec2f(0.f));
		ui::e_begin_layout(LayoutVertical)->get_component(cElement)->clip_children = true;
		ui::c_aligner(SizeFitParent, SizeFitParent);
		c_text_log = ui::e_text(app.filepath.c_str())->get_component(cText);
		ui::e_end_layout();
	ui::e_end_scroll_view1(ui::style_1u(ui::FontSize));

	ui::e_button(L"Clear", [](void* c) {
		app.console->c_text_log->set_text(L"");
	}, Mail<>());

	ui::e_begin_layout(LayoutHorizontal, 4.f);
	ui::c_aligner(SizeFitParent, SizeFixed);
		c_edit_input = ui::e_edit(0.f)->get_component(cEdit);
		ui::e_button(L"Exec", [](void* c) {
			auto log_text = app.console->c_text_log;
			std::wstring log = log_text->text();
			auto input_text = app.console->c_edit_input->text;
			auto cmd = std::wstring(input_text->text());
			log += cmd + L"\n";
			input_text->set_text(L"");
			app.console->c_edit_input->cursor = 0;

			auto tokens = SUW::split(cmd);

			if (app.locked)
				log += L"bp is locked\n";
			else
			{
				extra_global_db_count = app.bp->db_count();
				extra_global_dbs = app.bp->dbs();

				auto set_data = [&](const std::string& address, const std::string& value) {
					auto i = app.bp->find_input(address.c_str());
					if (i)
					{
						auto type = i->type();
						auto value_before = type->serialize(i->data(), 2);
						auto data = new char[i->size()];
						type->unserialize(value, data);
						i->set_data((char*)data);
						if (app.bp_editor)
							app.bp_editor->on_data_changed(i);
						delete[] data;
						auto value_after = type->serialize(i->data(), 2);
						log += L"set value: " + s2w(address) + L", " + s2w(value_before) + L" -> " + s2w(value_after) + L"\n";
						app.set_changed(true);
					}
					else
						log += L"input not found\n";
				};

				if (tokens[0] == L"help")
				{
					log +=
						L"  help - show this help\n"
						"  show udts - show all available udts (see blueprint.h for more details)\n"
						"  show udt [udt_name] - show an udt\n"
						"  show nodes - show all nodes\n"
						"  show node [id] - show a node\n"
						"  show graph - use Graphviz to show graph\n"
						"  add node [type_name] [id] - add a node (id of '-' means don't care)\n"
						"  add link [out_adress] [in_adress] - add a link\n"
						"  remove node [id] - remove a node\n"
						"  remove link [in_adress] - remove a link\n"
						"  set [in_adress] [value] - set value for input\n"
						"  update - update this blueprint\n"
						"  save - save this blueprint\n"
						"  auto-set-layout - set nodes' positions using 'bp.png' and 'bp.graph.txt', need do show graph first\n";;
				}
				else if (tokens[0] == L"show")
				{
					if (tokens[1] == L"udts")
					{
						std::vector<UdtInfo*> all_udts;
						for (auto i = 0; i < global_db_count(); i++)
						{
							auto udts = global_db(i)->get_udts();
							for (auto i = 0; i < udts.s; i++)
								all_udts.push_back(udts.v[i]);
						}
						for (auto i = 0; i < app.bp->db_count(); i++)
						{
							auto udts = app.bp->dbs()[i]->get_udts();
							for (auto i = 0; i < udts.s; i++)
								all_udts.push_back(udts.v[i]);
						}
						std::sort(all_udts.begin(), all_udts.end(), [](UdtInfo* a, UdtInfo* b) {
							return std::string(a->type()->name()) < std::string(b->type()->name());
						});
						for (auto udt : all_udts)
							log += s2w(udt->type()->name()) + L"\n";
					}
					else if (tokens[1] == L"udt")
					{
						auto udt = find_udt(FLAME_HASH(w2s(tokens[2]).c_str()));
						if (udt)
						{
							log += s2w(udt->type()->name()) + L"\n";
							std::vector<VariableInfo*> inputs;
							std::vector<VariableInfo*> outputs;
							for (auto i_i = 0; i_i < udt->variable_count(); i_i++)
							{
								auto vari = udt->variable(i_i);
								auto flags = vari->flags();
								if (flags & VariableFlagInput)
									inputs.push_back(vari);
								else if (flags & VariableFlagOutput)
									outputs.push_back(vari);
							}
							log += L"[In]\n";
							for (auto& i : inputs)
								log += wfmt(L"name:%s flags:%d type:%s", s2w(i->name()).c_str(), i->flags(), s2w(i->type()->name()).c_str()) + L"\n";
							log += L"[Out]\n";
							for (auto& o : outputs)
								log += wfmt(L"name:%s flags:%d type:%s", s2w(o->name()).c_str(), o->flags(), s2w(o->type()->name()).c_str()) + L"\n";
						}
						else
							log += L"udt not found\n";
					}
					else if (tokens[1] == L"nodes")
					{
						for (auto i = 0; i < app.bp->node_count(); i++)
						{
							auto n = app.bp->node(i);
							log += wfmt(L"id:%s type:%s", s2w(n->id()).c_str(), s2w(n->udt()->type()->name()).c_str()) + L"\n";
						}
					}
					else if (tokens[1] == L"node")
					{
						auto n = app.bp->find_node(w2s(tokens[2]).c_str());
						if (n)
						{
							log += L"[In]\n";
							for (auto i = 0; i < n->input_count(); i++)
							{
								auto input = n->input(i);
								auto type = input->type();
								log += s2w(input->name()) + L"\n";
								std::string link_address;
								if (input->link())
									link_address = input->link()->get_address().str();
								log += wfmt(L"[%s]", s2w(link_address).c_str()) + L"\n";
								auto str = s2w(type->serialize(input->data(), 2));
								if (str.empty())
									str = L"-";
								log += wfmt(L"   %s", str.c_str()) + L"\n";
							}
							log += L"[Out]\n";
							for (auto i = 0; i < n->output_count(); i++)
							{
								auto output = n->output(i);
								auto type = output->type();
								log += s2w(output->name()) + L"\n";
								auto str = s2w(type->serialize(output->data(), 2).c_str());
								if (str.empty())
									str = L"-";
								log += wfmt(L"   %s", str.c_str()) + L"\n";
							}
						}
						else
							log += L"node not found\n";
					}
					else if (tokens[1] == L"graph")
					{
						if (!app.generate_graph_image())
						{
							exec(L"app.bp.png", L"", false);
							log += L"ok\n";
						}
						else
							log += L"app.bp.png not found, perhaps Graphviz is not available\n";
					}
					else
						log += L"unknow object to show\n";
				}
				else if (tokens[0] == L"add")
				{
					if (tokens[1] == L"node")
					{
						auto n = app.add_node(w2s(tokens[2]).c_str(), (tokens[3] == L"-" ? "" : w2s(tokens[3])).c_str());
						n->pos = Vec2f(0.f);
						if (n)
							log += wfmt(L"node added: %s", s2w(n->id()).c_str()) + L"\n";
						else
							log += L"bad udt name or id already exist\n";
					}
					else if (tokens[1] == L"link")
					{
						auto out = app.bp->find_output(w2s(tokens[2]).c_str());
						auto in = app.bp->find_input(w2s(tokens[3]).c_str());
						if (out && in)
						{
							in->link_to(out);
							auto out_addr = in->link()->get_address();
							auto in_addr = in->get_address();
							log += wfmt(L"link added: %s -> %s", s2w(out_addr.str()).c_str(), s2w(in_addr.str()).c_str()) + L"\n";
							app.set_changed(true);
						}
						else
							log += L"wrong address\n";
					}
					else
						log += L"unknow object to add\n";
				}
				else if (tokens[0] == L"remove")
				{
					if (tokens[1] == L"node")
					{
						auto n = app.bp->find_node(w2s(tokens[2]).c_str());
						if (n)
						{
							if (!app.remove_node(n))
								printf("cannot remove test nodes\n");
							else
								log += wfmt(L"node removed: %s", tokens[2].c_str()) + L"\n";
						}
						else
							log += L"node not found\n";
					}
					else if (tokens[1] == L"link")
					{
						auto i = app.bp->find_input(w2s(tokens[3]).c_str());
						if (i)
						{
							i->link_to(nullptr);
							log += wfmt(L"link removed: %s", tokens[2].c_str()) + L"\n";
							app.set_changed(true);
						}
						else
							log += L"input not found\n";
					}
					else
						log += L"unknow object to remove\n";
				}
				else if (tokens[0] == L"set")
					set_data(w2s(tokens[1]), w2s(tokens[2]));
				else if (tokens[0] == L"update")
				{
					app.bp->update();
					log += L"BP updated\n";
				}
				else if (tokens[0] == L"save")
				{
					BP::save_to_file(app.bp, app.filepath.c_str());
					app.set_changed(false);
					log += L"file saved\n";
				}
				else if (tokens[0] == L"auto-set-layout")
				{
					if (app.auto_set_layout())
						log += L"ok\n";
					else
						log += L"app.bp.graph.txt not found\n";
				}
				else
					log += L"unknow command\n";

				extra_global_db_count = 0;
				extra_global_dbs = nullptr;
			}

			log_text->set_text(log.c_str());

		}, Mail<>());
	ui::e_end_layout();
}

cConsole::~cConsole()
{
	app.console = nullptr;
}