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
