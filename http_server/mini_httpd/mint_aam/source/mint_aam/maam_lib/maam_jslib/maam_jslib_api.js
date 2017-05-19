// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintAAM.
// Some rights reserved. See README.

var maam_return_code = new function()
{
    this.pass                                  = 0;

    this.internal_error                        = -2000;
    this.session_table_full                    = -2001;

    this.miss_verify_name                      = -2100;
    this.miss_verify_password                  = -2101;
    this.miss_multiple_user_active_rule        = -2102,
    this.miss_same_accout_multiple_active_rule = -2103,
    this.miss_session_key                      = -2104;

    this.invalid_account_name                  = -2200;
    this.invalid_account_password              = -2201;
    this.other_user_has_login                  = -2202,
    this.same_account_has_login                = -2203,
    this.invalid_session_key                   = -2204;

    this.idle_timeout                          = -2300;

    this.jslib_internal_error                  = -2400;

    this.boundary                              = -2500;
};

var maaam_request_action = new function()
{
    this.none   = 0;
    this.login  = 1;
    this.logout = 2;
    this.check  = 3;
};

var maam_limit_multiple_active = new function()
{
    this.allow                        = 0;
    this.deny_force_kick_inside       = 1;
    this.deny_force_kick_outside      = 2;
    this.deny_permission_kick_inside  = 3;
    this.deny_permission_kick_outside = 4;
};

function maam_jslib_lib_t()
{
    this.request_action = 0;
    this.request_data   = "";
    this.other_query    = "";
};

function maam_split_response(
    rep_con)
{
    var split_loc, rep_code = maam_return_code.jslib_internal_error, rep_data = "";


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

function maam_jslib_run_script(
    script_con)
{
    if(script_con.length > 0)
    {
        script_con = "<script type=\"text/javascript\">" + script_con + "</script>";
        $("head").append(script_con);
    }
}

function maam_jslib_account_auth(
    this_maam_jslib)
{
    var jq_option, rep_code = maam_return_code.jslib_internal_error, rep_data = null;
    var path_con;


    if(this_maam_jslib.request_action == maaam_request_action.login)
        path_con = "/account-login";
    else
    if(this_maam_jslib.request_action == maaam_request_action.logout)
        path_con = "/account-logout";
    else
    if(this_maam_jslib.request_action == maaam_request_action.check)
        path_con = "/account-check";

    if(this_maam_jslib.other_query.length > 0)
        path_con += "?" + this_maam_jslib.other_query;

    jq_option =
    {
        async: false,
        dataType: "text",
        type: "POST",
        url: path_con,
        data: this_maam_jslib.request_data,

        success: function(data, textStatus, jqXHR)
        {
            var rep_con = maam_split_response(data);

            rep_code = rep_con[0];
            rep_data = rep_con[1];
        },
        error: function(jqXHR, textStatus, errorThrown)
        {
            alert("ajax error (maam_jslib_account_auth)\n" +
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
