The site's hosted on my laptop, so it might not be up all the time. <br />
Website URL: https://www.purplesite.skin
<br /> <br />
If you want to run it yourself (only works on Linux): <br />
* Install GCC
* Install the openssl library, openssl requires perl be installed first <br />
* Get some kind of certification, LetsEncrypt is easy and free, or change the SSL parts of the code <br />
* Port forward it <br />
* Run "make" in the PurpleSite directory <br />
* Then enter "sudo ./https_server port". By default the server runs on port 443, which you might need to allow
