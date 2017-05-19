// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

function jsapi_make_table(
    table_css,
    td_css,
    data_array)
{
    var tmp_str, row_cnt, col_cnt, i, j;


    col_cnt = data_array.length;
    row_cnt = data_array[0].length;

    tmp_str = "<table";
    if(table_css.length > 0)
        tmp_str += " class=\"" + table_css + "\"";
    tmp_str += ">";

    for(i = 0; i < row_cnt; i++)
    {
        tmp_str += "<tr>";

        for(j = 0; j < col_cnt; j++)
        {
            tmp_str += "<td";
            if(td_css.length > 0)
                tmp_str += " class=\"" + td_css + "\"";
            tmp_str += ">";

            tmp_str += data_array[j][i];

            tmp_str += "</td>";
        }

        tmp_str += "</tr>";
    }

    tmp_str += "</table>";

    return tmp_str;
}

function jsapi_make_trtd(
    td_css,
    data_array)
{
    var tmp_str, col_cnt, i;


    col_cnt = data_array.length;

    tmp_str = "<tr>";

    for(i = 0; i < col_cnt; i++)
    {
        tmp_str += "<td";
        if(td_css.length > 0)
            tmp_str += " class=\"" + td_css + "\"";
        tmp_str += ">";

        tmp_str += data_array[i];

        tmp_str += "</td>";
    }

    tmp_str += "</tr>";

    return tmp_str;
}

function jsapi_make_text(
    text_id,
    text_size,
    text_value)
{
    var tmp_str;


    tmp_str = "<input";
    if(text_id.length > 0)
        tmp_str += " id=\"" + text_id + "\"";
    tmp_str += " type=\"text\"";
    if(text_size > 0)
        tmp_str += " size=\"" + text_size + "\"";
    tmp_str += " value=\"" + text_value + "\">";

    return tmp_str;
}

function jsapi_make_checkbox(
    checkbox_id,
    checkbox_disable)
{
    var tmp_str;


    tmp_str = "<input";
    if(checkbox_id.length > 0)
        tmp_str += " id=\"" + checkbox_id + "\"";
    tmp_str += " type=\"checkbox\"";
    if(checkbox_disable != 0)
        tmp_str += " disabled=\"disabled\"";
    tmp_str += "\">";

    return tmp_str;
}

function jsapi_make_select(
    select_id,
    option_set,
    option_add,
    option_del)
{
    var tmp_str;


    tmp_str = "<select";
    if(select_id.length > 0)
        tmp_str += " id=\"" + select_id + "\"";
    tmp_str += ">";

    tmp_str += "<option value=\"none\"></option>";
    if(option_set != 0)
        tmp_str += "<option value=\"set\">set</option>";
    if(option_add != 0)
        tmp_str += "<option value=\"add\">add</option>";
    if(option_del != 0)
        tmp_str += "<option value=\"del\">del</option>";

    tmp_str += "</select>";

    return tmp_str;
}

function jsapi_make_button(
    button_value,
    button_onclick)
{
    var tmp_str;


    tmp_str = "<input type=\"button\"";
    tmp_str += " value=\"" + button_value + "\"";
    tmp_str += " onclick=\"" + button_onclick + "\">";

    return tmp_str;
}

function jsapi_make_empty_table(
    table_id,
    table_css)
{
    var tmp_str;


    tmp_str = "<table";
    if(table_id.length > 0)
        tmp_str += " id=\"" + table_id + "\"";
    if(table_css.length > 0)
        tmp_str += " class=\"" + table_css + "\"";
    tmp_str += ">";

    tmp_str += "<thead></thead><tbody></tbody>";

    tmp_str += "</table>";

    return tmp_str;
}
