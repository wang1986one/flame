class Node
{
    constructor(desc)
    {
        var thiz = this;

        this.id = desc.id;
        this.type = desc.type;
    
        this.eMain = document.createElement("div");
        this.eMain.classList.add("node");
        this.eMain.setAttribute("title", this.id);
    
        var pos_sp = desc.pos.split(";");
        this.x = parseInt(pos_sp[0]);
        this.y = parseInt(pos_sp[1]);
        this.eMain.style.left = this.x.toString() + "px";
        this.eMain.style.top = this.y.toString() + "px";
    
        var thiz = this;
        $(this.eMain).draggable({
            containment: "window",
            cancel: ".slot, .slot_edit1, .slot_edit2",
            drag: function (event, ui) {
                thiz.update_links_positions();
            }
        });
    
        this.eLeft = document.createElement("div");
        this.eLeft.style.display = "inline-block";
        this.eLeft.style.marginRight = "30px";
        this.eMain.appendChild(this.eLeft);
    
        this.eRight = document.createElement("div");
        this.eRight.style.display = "inline-block";
        this.eRight.style.float = "right";
        this.eMain.appendChild(this.eRight);
    
        this.btnClose = document.createElement("button");
        this.btnClose.type = "button";
        this.btnClose.innerHTML = "X";
        this.btnClose.style.position = "absolute";
        this.btnClose.style.left = "0px";
        this.btnClose.style.top = "-20px";
        this.btnClose.onclick = function(){
            remove_node(thiz);
        };
        this.eMain.appendChild(this.btnClose);
    
        this.inputs = [];
        this.outputs = [];
    
        let sp = desc.type.split("#");
        function load(u_name)
        {
            var udt = find_udt(u_name);
            if (!udt)
                return false;
    
            thiz.udt = udt;
            for (let i in udt.items)
            {
                var item = udt.items[i];
                if (item.attribute.indexOf("i") >= 0)
                {
                    var s = new Slot(item, 0);
                    s.node = thiz;
                    if (item.default_value)
                        s.set_data(item.default_value);
                    else
                        s.data = "";
                    thiz.inputs.push(s);
                    thiz.eLeft.appendChild(s.eMain);
                }
                else if (item.attribute.indexOf("o") >= 0)
                {
                    var s = new Slot(item, 1);
                    s.node = thiz;
                    thiz.outputs.push(s);
                    thiz.eRight.appendChild(s.eMain);
                }
            }
            for (let i in desc.datas)
            {
                var item = desc.datas[i];
                thiz.find_input(item.name).set_data(item.value);
            }
            for (let i = 0; i < staging_links.length; i++)
            {
                var sl = staging_links[i];
    
                var addr_out = sl.out.split(".");
                var addr_in = sl.in.split(".");
    
                var n1 = find_node(addr_out[0]);
                var n2 = find_node(addr_in[0]);
                if (!n1 || !n2)
                    continue;
                var output = n1.find_output(addr_out[1]);
                var input = n2.find_input(addr_in[1]);
    
                if (input && output)
                {
                    input.links[0] = output;
                    output.links.push(input);
    
                    n1.update_links_positions();
                    n2.update_links_positions();
    
                    staging_links.splice(i, 1);
                    i--;
                }
            }
            return true;
        };
        if (sp.length == 1)
            console.assert(load(sp[0]));
        else
        {
            if (!load(sp[1]))
            {
                var url = filepath + "/" + sp[0] + ".typeinfo";
                request(url, function(res){
                    load_typeinfo(res, sp[0]);
                    console.assert(load(sp[1]));
                });
            }
        }
    }

    find_input(name)
    {
        for (let i in this.inputs)
        {
            var s = this.inputs[i];
            if (s.vi.name == name)
                return s;
        }
        return null;
    }

    find_output(name)
    {
        for (let i in this.outputs)
        {
            var s = this.outputs[i];
            if (s.vi.name == name)
                return s;
        }
        return null;
    }

    update_links_positions()
    {
        this.x = parseInt(this.eMain.style.left);
        this.y = parseInt(this.eMain.style.top);
    
        for (let i in this.inputs)
        {
            var s = this.inputs[i];
            if (s.links[0])
            {
                var a = s.get_pos();
                var b = s.links[0].get_pos();
        
                s.set_path(a, b);
            }
        }
        for (let i in this.outputs)
        {
            var s = this.outputs[i];
            for (let j in s.links)
            {
                var t = s.links[j];
    
                var a = t.get_pos();
                var b = s.get_pos();
        
                t.set_path(a, b);
            }
        }
    }
}
