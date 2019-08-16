#include <flame/foundation/serialize.h>
#include <flame/foundation/blueprint.h>
#include <flame/foundation/window.h>
#include <flame/network/network.h>
#include <flame/graphics/device.h>
#include <flame/graphics/synchronize.h>
#include <flame/graphics/swapchain.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/font.h>
#include <flame/graphics/canvas.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/event_dispatcher.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/checkbox.h>
#include <flame/universe/components/toggle.h>

using namespace flame;
using namespace graphics;

struct App
{
	Window* w;
	Device* d;
	Semaphore* render_finished;
	SwapchainResizable* scr;
	Fence* fence;
	std::vector<Commandbuffer*> cbs;

	FontAtlas* font_atlas;
	Canvas* canvas;
	int rt_frame;

	Entity* root;
	cElement* c_element_root;
	cText* c_text_fps;

	std::wstring filename;
	BP* bp;

	void* ev_1;
	void* ev_2;
	void* ev_3;

	OneClientServer* server;

	void run()
	{
		auto sc = scr->sc();
		auto sc_frame = scr->sc_frame();

		if (sc_frame > rt_frame)
		{
			canvas->set_render_target(TargetImages, sc ? &sc->images() : nullptr);
			rt_frame = sc_frame;
		}

		if (sc)
		{
			sc->acquire_image();
			fence->wait();

			c_element_root->width = w->size.x();
			c_element_root->height = w->size.y();
			c_text_fps->set_text(std::to_wstring(app_fps()));
			root->update();
			//bp->update();

			auto img_idx = sc->image_index();
			auto cb = cbs[img_idx];
			canvas->record(cb, img_idx);

			d->gq->submit(cb, sc->image_avalible(), render_finished, fence);
			d->gq->present(sc, render_finished);
		}

		if (wait_event(ev_1, 0))
		{
			fence->wait();

			set_event(ev_2);
			wait_event(ev_3, -1);
		}
	}

	void set_data(const std::string& address, const std::string& value)
	{
		auto i = bp->find_input(address.c_str());
		if (i)
		{
			set_event(ev_1);
			wait_event(ev_2, -1);

			auto v = i->variable_info();
			auto type = v->type();
			auto value_before = serialize_value(type->tag(), type->hash(), i->data(), 2);
			auto data = new char[v->size()];
			unserialize_value(type->tag(), type->hash(), value, data);
			i->set_data(data);
			delete data;
			auto value_after = serialize_value(type->tag(), type->hash(), i->data(), 2);
			printf("set value: %s, %s -> %s\n", address.c_str(), *value_before.p, *value_after.p);
			delete_mail(value_before);
			delete_mail(value_after);

			set_event(ev_3);
		}
		else
			printf("input not found\n");
	}

	void generate_graph_and_layout()
	{
		if (GRAPHVIZ_PATH == std::string(""))
			assert(0);
		auto dot_path = s2w(GRAPHVIZ_PATH) + L"/bin/dot.exe";

		std::string gv = "digraph bp {\nrankdir=LR\nnode [shape = Mrecord];\n";
		for (auto i = 0; i < bp->node_count(); i++)
		{
			auto src = bp->node(i);
			auto name = std::string(src->id());

			auto n = "\t" + name + " [label = \"" + name + "|{{";
			for (auto j = 0; j < src->input_count(); j++)
			{
				auto input = src->input(j);
				auto name = std::string(input->variable_info()->name());
				n += "<" + name + ">" + name;
				if (j != src->input_count() - 1)
					n += "|";
			}
			n += "}|{";
			for (auto j = 0; j < src->output_count(); j++)
			{
				auto output = src->output(j);
				auto name = std::string(output->variable_info()->name());
				n += "<" + name + ">" + name;
				if (j != src->output_count() - 1)
					n += "|";
			}
			n += "}}\"];\n";

			gv += n;
		}
		for (auto i = 0; i < bp->node_count(); i++)
		{
			auto src = bp->node(i);

			for (auto j = 0; j < src->input_count(); j++)
			{
				auto input = src->input(j);
				if (input->link())
				{
					auto in_addr = input->get_address();
					auto out_addr = input->link()->get_address();
					auto in_sp = string_split(*in_addr.p, '.');
					auto out_sp = string_split(*out_addr.p, '.');
					delete_mail(in_addr);
					delete_mail(out_addr);

					gv += "\t" + out_sp[0] + ":" + out_sp[1] + " -> " + in_sp[0] + ":" + in_sp[1] + ";\n";
				}
			}
		}
		gv += "}\n";

		std::ofstream file("bp.gv");
		file << gv;
		file.close();

		exec(dot_path, L"-Tpng bp.gv -o bp.png", true);
		exec(dot_path, L"-Tplain bp.gv -y -o bp.graph.txt", true);
	}

}app;
auto papp = &app;

int main(int argc, char **args)
{
	typeinfo_load(L"flame_foundation.typeinfo");
	typeinfo_load(L"flame_graphics.typeinfo");

	if (argc != 2)
	{
		printf("argc is not 2, exit\n");
		return 0;
	}

	app.filename = s2w(args[1]);
	app.bp = BP::create_from_file(app.filename);
	if (!app.bp)
	{
		printf("bp not found, exit\n");
		return 0;
	}

	app.ev_1 = create_event(false);
	app.ev_2 = create_event(false);
	app.ev_3 = create_event(false);

	std::thread([&]() {
		app.w = Window::create("Editor", Vec2u(1280, 720), WindowFrame | WindowResizable);
		app.d = Device::create(true);
		app.render_finished = Semaphore::create(app.d);
		app.scr = SwapchainResizable::create(app.d, app.w);
		app.fence = Fence::create(app.d);
		auto sc = app.scr->sc();
		app.canvas = Canvas::create(app.d, TargetImages, &sc->images());
		app.cbs.resize(sc->images().size());
		for (auto i = 0; i < app.cbs.size(); i++)
			app.cbs[i] = Commandbuffer::create(app.d->gcp);

		auto font = Font::create(L"c:/windows/fonts/msyh.ttc", 14);
		app.font_atlas = FontAtlas::create(app.d, FontDrawPixel, { font });

		app.root = Entity::create();
		{
			app.c_element_root = cElement::create(app.root, app.canvas);

			cEventDispatcher::create(app.root, app.w);

			cLayout::create(app.root);
		}

		auto e_fps = Entity::create();
		{
			cElement::create(e_fps);

			auto c_text = cText::create(e_fps, app.font_atlas);
			app.c_text_fps = c_text;

			auto c_aligner = cAligner::create(e_fps);
			c_aligner->x_align = AlignxLeft;
			c_aligner->y_align = AlignyBottom;
		}
		app.root->add_child(e_fps);

		set_event(app.ev_1);
		wait_event(app.ev_2, -1);
		app_run([](void* c) {
			auto app = (*(App**)c);
			app->run();
		}, new_mail_p(&app));
	}).detach();

	wait_event(app.ev_1, -1);

	set_event(app.ev_2);

	printf("\"%s\":\n", w2s(app.filename).c_str());

	network_init();

	std::vector<UdtInfo*> available_udts;
	{
		auto udts = get_udts();
		for (auto i = 0; i < udts.p->size(); i++)
			available_udts.push_back((*udts.p)[i]);
		delete_mail(udts);
		std::sort(available_udts.begin(), available_udts.end(), [](UdtInfo* a, UdtInfo* b) {
			return std::string(a->name()) < std::string(b->name());
		});
	}

	while (true)
	{
		char command_line[260];
		scanf("%s", command_line);
		auto s_command_line = std::string(command_line);
		if (s_command_line == "help")
		{
			printf(
				"  help - show this help\n"
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
				"  set-layout - set nodes' positions using 'bp.png' and 'bp.graph.txt', need do show graph first\n"
				"  gui-browser - use the power of browser to show and edit\n"
			);
		}
		else if (s_command_line == "show")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "udts")
			{
				for (auto u : available_udts)
					printf("%s\n", u->name());
			}
			else if (s_what == "udt")
			{
				scanf("%s", command_line);
				auto s_name = std::string(command_line);

				auto udt = find_udt(H(s_name.c_str()));
				if (udt)
				{
					printf("%s:\n", udt->name());
					std::vector<VariableInfo*> inputs;
					std::vector<VariableInfo*> outputs;
					for (auto i_i = 0; i_i < udt->variable_count(); i_i++)
					{
						auto vari = udt->variable(i_i);
						auto attribute = std::string(vari->decoration());
						if (attribute.find('i') != std::string::npos)
							inputs.push_back(vari);
						if (attribute.find('o') != std::string::npos)
							outputs.push_back(vari);
					}
					printf("[In]\n");
					for (auto &i : inputs)
						printf(" name:%s attribute:%s tag:%s type:%s\n", i->name(), i->decoration(), get_name(i->type()->tag()), i->type()->name());
					printf("[Out]\n");
					for (auto &i : outputs)
						printf(" name:%s attribute:%s tag:%s type:%s\n", i->name(), i->decoration(), get_name(i->type()->tag()), i->type()->name());
				}
				else
					printf("udt not found\n");
			}
			else if (s_what == "nodes")
			{
				for (auto i = 0; i < app.bp->node_count(); i++)
				{
					auto n = app.bp->node(i);
					printf("id:%s type:%s\n", n->id(), n->udt()->name());
				}
			}
			else if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->find_node(s_id.c_str());
				if (n)
				{
					printf("[In]\n");
					for (auto i = 0; i < n->input_count(); i++)
					{
						auto input = n->input(i);
						auto v = input->variable_info();
						auto type = v->type();
						printf(" %s\n", v->name());
						Mail<std::string> link_address;
						if (input->link())
							link_address = input->link()->get_address();
						printf("   [%s]->\n", link_address.p ? link_address.p->c_str() : "");
						delete_mail(link_address);
						auto str = serialize_value(type->tag(), type->hash(), input->data(), 2);
						printf("   %s\n", str.p->empty() ? "-" : str.p->c_str());
						delete_mail(str);
					}
					printf("[Out]\n");
					for (auto i = 0; i < n->output_count(); i++)
					{
						auto output = n->output(i);
						auto v = output->variable_info();
						auto type = v->type();
						printf(" %s\n", output->variable_info()->name());
						auto str = serialize_value(type->tag(), type->hash(), output->data(), 2);
						printf("   %s\n", str.p->empty() ? "-" : str.p->c_str());
						delete_mail(str);
					}
				}
				else
					printf("node not found\n");
			}
			else if (s_what == "graph")
			{
				if (!std::filesystem::exists(L"bp.png") || std::filesystem::last_write_time(L"bp.png") < std::filesystem::last_write_time(app.filename))
					app.generate_graph_and_layout();
				if (std::filesystem::exists(L"bp.png"))
				{
					exec(L"bp.png", L"", false);
					printf("ok\n");
				}
				else
					printf("bp.png not found, perhaps Graphviz is not available\n");
			}
			else
				printf("unknow object to show\n");
		}
		else if (s_command_line == "add")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_tn = std::string(command_line);

				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->add_node(s_tn.c_str(), s_id == "-" ? nullptr : s_id.c_str());
				if (!n)
					printf("bad udt name or id already exist\n");
				else
					printf("node added: %s\n", n->id());
			}
			else if (s_what == "link")
			{
				scanf("%s", command_line);
				auto s_out_address = std::string(command_line);

				scanf("%s", command_line);
				auto s_in_address = std::string(command_line);

				auto out = app.bp->find_output(s_out_address.c_str());
				auto in = app.bp->find_input(s_in_address.c_str());
				if (out && in)
				{
					in->link_to(out);
					auto out_addr = in->link()->get_address();
					auto in_addr = in->get_address();
					printf("link added: %s -> %s\n", out_addr.p->c_str(), in_addr.p->c_str());
					delete_mail(out_addr);
					delete_mail(in_addr);
				}
				else
					printf("wrong address\n");
			}
			else
				printf("unknow object to add\n");
		}
		else if (s_command_line == "remove")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->find_node(s_id.c_str());
				if (n)
				{
					app.bp->remove_node(n);
					printf("node removed: %s\n", s_id.c_str());
				}
				else
					printf("node not found\n");
			}
			else if (s_what == "link")
			{
				scanf("%s", command_line);
				auto s_in_address = std::string(command_line);

				auto i = app.bp->find_input(s_in_address.c_str());
				if (i)
				{
					i->link_to(nullptr);
					printf("link removed: %s\n", s_in_address.c_str());
				}
				else
					printf("input not found\n");
			}
			else
				printf("unknow object to remove\n");
		}
		else if (s_command_line == "set")
		{
			scanf("%s", command_line);
			auto s_address = std::string(command_line);

			scanf("%s", command_line);
			auto s_value = std::string(command_line);

			app.set_data(s_address, s_value);
		}
		else if (s_command_line == "update")
		{
			app.bp->update();
			printf("BP updated\n");
		}
		else if (s_command_line == "save")
		{
		BP::save_to_file(app.bp, app.filename);
			printf("file saved\n");
		}
		else if (s_command_line == "set-layout")
		{
			if (!std::filesystem::exists(L"bp.graph.txt") || std::filesystem::last_write_time(L"bp.graph.txt") < std::filesystem::last_write_time(app.filename))
				app.generate_graph_and_layout();
			if (std::filesystem::exists(L"bp.graph.txt"))
			{
				auto str = get_file_string(L"bp.graph.txt");
				std::regex reg_node(R"(node ([\w]+) ([\d\.]+) ([\d\.]+))");
				std::smatch match;
				while (std::regex_search(str, match, reg_node))
				{
					auto n = app.bp->find_node(match[1].str().c_str());
					if (n)
						n->set_pos(Vec2f(std::stof(match[2].str().c_str()), std::stof(match[3].str().c_str())) * 100.f);

					str = match.suffix();
				}
				printf("ok\n");
			}
			else
				printf("bp.graph.txt not found\n");
		}
		else if (s_command_line == "gui-browser")
		{
			auto curr_path = get_curr_path();
			//exec(L"file:///" + *get_curr_path().p + L"/bp.html", L"", false);
			delete_mail(curr_path);
			printf("waiting for browser on port 5566 ...");

			app.server = OneClientServer::create(SocketWeb, 5566, 100, [](void* c, const std::string& str) {
				auto app = *(App * *)c;

				auto req = SerializableNode::create_from_json_string(str);
				auto type = req->find_attr("type")->value();

				if (type == "get")
				{
					auto filename = s2w(req->find_attr("filename")->value());
					if (filename == L"bp")
						filename = app->filename;
					auto file = base64_encode(get_file_string(filename));
					auto res = SerializableNode::create("");
					res->new_node("filename")->set_value(w2s(filename));
					res->new_node("data")->set_value(file);
					auto str = SerializableNode::to_json_string(res);
					app->server->send(str.p->size(), str.p->data());
					delete_mail(str);
					SerializableNode::destroy(res);
				}
				else if (type == "update")
				{
					auto what = req->find_attr("what")->value();

					if (what == "set_data")
						app->set_data(req->find_attr("address")->value(), req->find_attr("value")->value());
				}

				SerializableNode::destroy(req);
			}, new_mail(&papp));
			if (!app.server)
				printf("  timeout\n");
			else
			{
				printf("  ok\nbrowser: working\n");

				wait_event(app.server->ev_closed, -1);
				printf("browser: closed\n");
			}
		}
		else
			printf("unknow command\n");
	}

	return 0;
}

/*
	auto layout = Element::createT<wLayout>(ui, LayoutHorizontal);

	auto image1 = Element::createT<wImage>(ui);
	image1->size$ = Vec2f(250.f);
	image1->id() = 0;
	image1->align$ = AlignLittleEnd;

	auto image2 = Element::createT<wImage>(ui);
	image2->size$ = Vec2f(250.f);
	image2->id() = 0;
	image2->align$ = AlignLittleEnd;

	auto splitter = Element::createT<wSplitter>(ui, 0, image1, image2);
	root->add_child(splitter);

	layout->add_child(image1);
	layout->add_child(splitter);
	layout->add_child(image2);
	root->add_child(layout);

*/