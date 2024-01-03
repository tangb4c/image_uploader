# image_uploader
for quick sharing of images hosted on your own server

this is a nice work, thx arjanhouben.

# compile under centos7

```shell
sudo yum -y install devtoolset-10-gcc devtoolset-10-gcc-c++ devtoolset-10-binutils ImageMagick

git clone --depth 1 <current git repo>

cmake3 -DCMAKE_BUILD_TYPE=Release -DPASSWORD=password -S image_uploader -B image_uploader/cmake-build-release

make -C image_uploader/cmake-build-release

mv image_uploader/cmake-build-release/image_uploader_backend.cgi /data/www/
cp image_uploader/index.html /data/www/
```

# set up fast-cgi under nginx

run these commands in root: 

```
yum -y install fcgiwrap spawn-fcgi fcgi

vim /etc/sysconfig/spawn-fcgi

# add below config
FCGI_SOCKET=/var/run/fcgiwrap.sock
FCGI_PROGRAM=/usr/sbin/fcgiwrap
FCGI_USER=nginx
FCGI_GROUP=nginx
FCGI_EXTRA_OPTIONS="-M 0777"
OPTIONS="-u $FCGI_USER -g $FCGI_GROUP -s $FCGI_SOCKET -S $FCGI_EXTRA_OPTIONS -F 1 -P /var/run/spawn-fcgi.pid -- $FCGI_PROGRAM"

systemctl enable spawn-fcgi
systemctl start spawn-fcgi

vim nginx/conf/conf.d/b4c.host.conf
# add below config
location ~ .*\.(pl|py|cgi)?$ {
        include        fastcgi_params;
        fastcgi_pass   unix:/var/run/fcgiwrap.sock;
        # fastcgi_index  index.cgi;
        fastcgi_param  SCRIPT_FILENAME /data/www/$fastcgi_script_name;
}
systemctl restart nginx

```

# debug

```shell
export 'CONTENT_TYPE=multipart/form-data; boundary=----WebKitFormBoundaryvBLsA6RILOgjyW48'

```
