![](/usage/zh-TW/image/mcm_0101_0101.png "系統架構")

**`mcm_daemon`**  
資料管理的主程式, 提供設資料取服務, 使用 Unix Domain Socket 和其他程式溝通, 可以同時接受多個連線, 每個連線會建立一個執行緒處理, 讀取請求可以多個程式同時使用, 寫入請求同時只能一個程式使用 (讀寫鎖搭配先入先出列隊).  
**`[custom data handle]`**  
客制化的資料處理函式庫, 處理資料的運用, 其他程式修改資料後可以指定要執行哪個處理函式做處理, 例如其他程式要修改網路介面的位址, 先設定新的網路介面位址, 之後指定要執行資料處理函式庫內處理修改網路介面的函式, 修改網路介面的函式被 mcm_daemon 執行後, 取出新的網路介面位址, 在套用到網路介面上. 此部分會編譯成動態連結函式檔給 mcm_daemon 使用. 

**`libmcm_lulib_api.so`**  
提供介面 (C 函式) 給其他用戶端 (User Space) 程式做資料的存取. 

`User Space Program`  
需要資料存取服務的用戶端程式. 

**`mcm_lklib_api.ko`**  
提供介面 (C 函式) 給其他核心端 (Kernel Space) 程式做資料的存取.

`Kernel Space Program`  
需要資料存取服務的核心端程式. 

`mini_httpd`  
使用的 HTTP Server.

**`mcm_cgi_config.cgi`**  
處理網頁程式的資料存取, 此程式會自動處理網頁端的資料存取, 不需要針對每個資料表手動撰寫存取程式. 對於取得資料, 網頁端會使用 AJAX POST 告知要哪些資料表的資料, 此程式會和 mcm_daemon 溝通取出指定的資料並組合成 JSON 格式回傳給網頁. 對於修改資料, 網頁端會使用 AJAX POST 告知要修改哪些資料, 此程式再通知 mcm_daemon 要修改哪些資料.  
**`[custom config handle]`**  
自訂的資料過濾函式, 一般情況下讀取資料表的資料時會讀出資料表內全部的資料, 使用自訂的過濾函式可以指定只讀取資料表內的某幾筆資料, 此部分會編譯成動態連結函式檔給 mcm_cgi_config.cgi 使用.

**`mcm_cgi_upload.cgi`**  
處理網頁程式的檔案上傳, 使用上傳 form 處理, 支援使用 multipart/form-data 同時上傳 form 內的多個元素或檔案.  
**`[custom upload handle]`**  
自訂的檔案處理函式, 處理網頁程式上傳的檔案, 此部分會編譯成動態連結函式檔給 mcm_cgi_upload.cgi 使用.

**`mcm_jslib_api.js`**  
提供介面給網頁程式做資料的存取和檔案的上傳, 資料存取部分使用 AJAX + JSON 機制, 檔案上傳部分使用 form 機制.

`Web Program`  
需要資料存取服務的網頁端程式. 

**`mcm command`**  
指令程式, 在 Shell Script 中可以使用此指令程式做資料存取.

`Shell Script Program`  
需要資料存取服務的 Shell Script 端程式. 

詳細的使用說明檔在 `mint_cm/usage/zh-TW/mcm_index.html`

授權說明檔在 `mint_cm/README`
