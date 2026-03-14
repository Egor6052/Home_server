

This is ahome server app, working in autostart and collection the date about humidity and temperature.


// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";
// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
  apiKey: "AIzaSyAPJTbQ03YqUr-XkrodwyufgBAmfAJtD70",
  authDomain: "home-server-9e586.firebaseapp.com",
  projectId: "home-server-9e586",
  storageBucket: "home-server-9e586.firebasestorage.app",
  messagingSenderId: "816629930704",
  appId: "1:816629930704:web:9aafbfbf66519357ed1cd5",
  measurementId: "G-YWC1W33PM1"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);





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


