

This is ahome server app, working in autostart and collection the date about humidity and temperature.

Extansions:
```sh
sudo apt-get install v4l-utils

sudo apt-get install ffmpeg alsa-utils

sudo apt install nginx
```


## Nginx configuration
create a simlinc to project file
```sh
sudo ln -s /home/admin/Programs/Server/back/nginx.conf /etc/nginx/sites-enabled/homeserver

```

>The dist folder and all files in it must be readable:
```sh
chmod -R 755 /home/admin/Programs/Server/front/dist
```

>Check if Nginx sees your file and if there are no errors:
```sh
sudo nginx -t
```
Restart Nginx:
```sh
sudo systemctl reload nginx
```


---
## Daemon Management

##### Reload Daemons
```sh
sudo systemctl daemon-reload
```

##### Restart Daemon
```sh
sudo systemctl restart home_server.service
```

##### Check Daemon Status
```sh
sudo systemctl status home_server.service
```

##### Disable Daemon
```sh
sudo systemctl disable home_server.service
```

##### Enable Daemon
```sh
sudo systemctl enable home_server.service
```

##### Stop Daemon
```sh
sudo systemctl stop home_server.service
```
##### Start Daemon
```sh
sudo systemctl start home_server.service
```

##### Delete Daemon
```sh
sudo rm /etc/systemd/system/home_server.service
```


Перевірка пакетів на вхід/вихід:
```sh
socat -v - /dev/serial0,b115200,raw,echo=0
```


