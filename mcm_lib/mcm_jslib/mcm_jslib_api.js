// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

var mcm_return_code = new function()
{
    this.pass                      = 0;

    this.common_shutdown           = -1;

    this.daemon_internal_error     = -100;

    this.config_internal_error     = -200;
    this.config_alloc_fail         = -201;
    this.config_invalid_path       = -202;
    this.config_invalid_size       = -203;
    this.config_invalid_member     = -204;
    this.config_invalid_store      = -205;
    this.config_access_deny        = -206;
    this.config_duplic_entry       = -207;
    this.config_too_many_entry     = -208;

    this.service_internal_error    = -300;
    this.service_alloc_fail        = -301;
    this.service_invalid_req       = -302;

    this.action_internal_error     = -400;

    this.lulib_internal_error      = -500;
    this.lulib_alloc_fail          = -501;
    this.lulib_socket_error        = -502;
    this.lulib_interrupt           = -503;

    this.lklib_internal_error      = -600;
    this.lklib_alloc_fail          = -601;
    this.lklib_socket_error        = -602;

    this.jslib_internal_error      = -700;

    this.cgi_config_internal_error = -800;

    this.cgi_upload_internal_error = -900;

    this.command_internal_error    = -1000;

    this.boundary                  = -1100;

    this.module_internal_error     = -3000;
};

var mcm_request_action = new function()
{
    this.obtain_max_count = 0;
    this.obtain_config    = 1;
    this.submit_config    = 2;
};

var mcm_session_permission = new function()
{
    this.ro = 0;
    this.rw = 1;
};

var mcm_data_format = new function()
{
    this.all_default = 0x0000;
    this.ek_string   = 0x0001;
    this.rk_string   = 0x0002;
    this.isc_string  = 0x0004;
    this.iuc_string  = 0x0008;
    this.iss_string  = 0x0010;
    this.ius_string  = 0x0020;
    this.isi_string  = 0x0040;
    this.iui_string  = 0x0080;
    this.isll_string = 0x0100;
    this.iull_string = 0x0200;
    this.ff_string   = 0x0400;
    this.fd_string   = 0x0800;
    this.fld_string  = 0x1000;
    this.all_string  = this.ek_string | this.rk_string |
                       this.isc_string | this.iuc_string |
                       this.iss_string | this.ius_string |
                       this.isi_string | this.iui_string |
                       this.isll_string | this.iull_string |
                       this.ff_string | this.fd_string | this.fld_string;
};

var mcm_after_complete = new function()
{
    this.none   = 0;
    this.reload = 1;
};

function mcm_jslib_lib_t()
{
    this.socket_path        = "";
    this.session_permission = mcm_session_permission.rw;
    this.session_stack_size = 0;
    this.request_command    = "";
    this.data_format        = mcm_data_format.all_default;
    this.after_complete     = mcm_after_complete.none;
    this.other_query        = "";
};

function mcm_upload_jslib_t()
{
    this.form_id_string = "";
    this.other_query    = "";
};

function mcm_jslib_convert_html_str(
    data_con)
{
    var i, j, tmp_data = "", char_list;


    if(data_con != "")
    {
        char_list = [];
        char_list[char_list.length] = ['"', "&#0034;"];
        char_list[char_list.length] = ['&', "&#0038;"];
        char_list[char_list.length] = ['<', "&#0060;"];
        char_list[char_list.length] = ['>', "&#0062;"];
        char_list[char_list.length] = [' ', "&#0160;"];

        for(i = 0; i < data_con.length; i++)
        {
            for(j = 0; j < char_list.length; j++)
                if(data_con[i] == char_list[j][0])
                    break;
            tmp_data += j == char_list.length ? data_con.charAt(i) : char_list[j][1];
        }
    }

    return tmp_data;
}

function mcm_jslib_convert_submit_str(
    data_con)
{
    var didx, tmp_data = "";


    if(data_con != "")
        tmp_data = encodeURIComponent(data_con);

    return tmp_data;
}

function mcm_jslib_run_script(
    script_con)
{
    if(script_con.length > 0)
    {
        script_con = "<script type=\"text/javascript\">" + script_con + "</script>";
        $("head").append(script_con);
    }
}

function mcm_split_response(
    rep_con)
{
    var split_loc, rep_code = mcm_return_code.jslib_internal_error, rep_data = "";


    if(rep_con.length > 0)
    {
        // for IE8.
        if(rep_con.substr(0, 5) == "<PRE>")
        {
            rep_con = rep_con.substr(5);
            if(rep_con.substr(rep_con.length - 6, rep_con.length) == "</PRE>")
                rep_con = rep_con.substr(0, rep_con.length - 6);
        }

        if((split_loc = rep_con.indexOf(":")) > -1)
        {
            rep_code = parseInt(rep_con.substring(0, split_loc), 10);
            rep_data = rep_con.substring(split_loc + 1, rep_con.length);
        }
        else
        {
            rep_code = parseInt(rep_con);
        }
    }

    return [rep_code, rep_data];
}

function mcm_jslib_obtain_max_count(
    this_jslib)
{
    var jq_option, path_con, rep_code = mcm_return_code.jslib_internal_error, rep_data = null;


    path_con = "/cgi/mcm_cgi_config.cgi?" +
               "request_action=" + mcm_request_action.obtain_max_count + "&" +
               "socket_path=" + this_jslib.socket_path + "&" +
               "session_permission=" + this_jslib.session_permission + "&" +
               "session_stack_size=" + this_jslib.session_stack_size;
    if(this_jslib.other_query.length > 0)
        path_con += "&" + this_jslib.other_query;

    jq_option =
    {
        async: false,
        dataType: "text",
        type: "POST",
        url: path_con,
        data: this_jslib.request_command,

        success: function(data, textStatus, jqXHR)
        {
            var rep_con = mcm_split_response(data);

            rep_code = rep_con[0];
            rep_data = rep_con[1];
        },
        error: function(jqXHR, textStatus, errorThrown)
        {
            alert("ajax error (mcm_jslib_obtain_max_count)\n" +
                  "[" + jq_option.url + "]\n" +
                  "[" + jqXHR.status + "]\n" +
                  "[" + jqXHR.responseText + "]\n" +
                  "[" + textStatus + "]\n" +
                  "[" + errorThrown + "]");
        }
    }

    $.ajax(jq_option);

    return {rep_code:rep_code, rep_data:rep_data};
}

function mcm_jslib_obtain_config(
    this_jslib)
{
    var jq_option, path_con, rep_code = mcm_return_code.jslib_internal_error, rep_data = null;


    path_con = "/cgi/mcm_cgi_config.cgi?" +
               "request_action=" + mcm_request_action.obtain_config + "&" +
               "socket_path=" + this_jslib.socket_path + "&" +
               "session_permission=" + this_jslib.session_permission + "&" +
               "session_stack_size=" + this_jslib.session_stack_size + "&" +
               "data_format=" + this_jslib.data_format;
    if(this_jslib.other_query.length > 0)
        path_con += "&" + this_jslib.other_query;

    jq_option =
    {
        async: false,
        dataType: "text",
        type: "POST",
        url: path_con,
        data: this_jslib.request_command,

        success: function(data, textStatus, jqXHR)
        {
            var rep_con = mcm_split_response(data);

            rep_code = rep_con[0];
            rep_data = rep_con[1];
        },
        error: function(jqXHR, textStatus, errorThrown)
        {
            alert("ajax error (mcm_jslib_obtain_config)\n" +
                  "[" + jq_option.url + "]\n" +
                  "[" + jqXHR.status + "]\n" +
                  "[" + jqXHR.responseText + "]\n" +
                  "[" + textStatus + "]\n" +
                  "[" + errorThrown + "]");
        }
    }

    $.ajax(jq_option);

    return {rep_code:rep_code, rep_data:rep_data};
}

function mcm_jslib_submit_config(
    this_jslib)
{
    var jq_option, path_con, rep_code = mcm_return_code.jslib_internal_error, rep_data = null;


    path_con = "/cgi/mcm_cgi_config.cgi?" +
               "request_action=" + mcm_request_action.submit_config + "&" +
               "socket_path=" + this_jslib.socket_path + "&" +
               "session_permission=" + this_jslib.session_permission + "&" +
               "session_stack_size=" + this_jslib.session_stack_size + "&" +
               "after_complete=" + this_jslib.after_complete;
    if(this_jslib.other_query.length > 0)
        path_con += "&" + this_jslib.other_query;

    jq_option =
    {
        async: false,
        dataType: "text",
        type: "POST",
        url: path_con,
        data: this_jslib.request_command,

        success: function(data, textStatus, jqXHR)
        {
            var rep_con = mcm_split_response(data);

            rep_code = rep_con[0];
            rep_data = rep_con[1];
        },
        error: function(jqXHR, textStatus, errorThrown)
        {
            alert("ajax error (mcm_jslib_submit_config)\n" +
                  "[" + jq_option.url + "]\n" +
                  "[" + jqXHR.status + "]\n" +
                  "[" + jqXHR.responseText + "]\n" +
                  "[" + textStatus + "]\n" +
                  "[" + errorThrown + "]");
        }
    }

    $.ajax(jq_option);

    return {rep_code:rep_code, rep_data:rep_data};
}

function mcm_jslib_upload(
    this_upload_jslib)
{
    var jq_option, path_con, rep_code = mcm_return_code.jslib_internal_error, rep_data = null;


    path_con = "/cgi/mcm_cgi_upload.cgi";
    if(this_upload_jslib.other_query.length > 0)
        path_con += "?" + this_upload_jslib.other_query;

    jq_option =
    {
        async: false,
        type: "POST",
        url: path_con,

        success: function(responseText, textStatus)
        {
            var rep_con = mcm_split_response(responseText);

            rep_code = rep_con[0];
            rep_data = rep_con[1];
        },
        error: function()
        {
            alert("internal error (call ajaxSubmit() fail)");
        }
    }

    $("#" + this_upload_jslib.form_id_string).ajaxSubmit(jq_option);

    return {rep_code:rep_code, rep_data:rep_data};
}
