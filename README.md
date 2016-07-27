# tws
这是对<a href="http://csapp.cs.cmu.edu/">《深入理解计算机系统》</a>一书的第三部分“程序间的交互和通信”内容的一次实践（相应的读书笔记见<a href="https://github.com/woojean/woojean.github.io/blob/master/blogs/%E3%80%8A%E6%B7%B1%E5%85%A5%E7%90%86%E8%A7%A3%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%B3%BB%E7%BB%9F%E3%80%8B%E8%AF%BB%E4%B9%A6%E7%AC%94%E8%AE%B0.md">这里</a>）。实现了一个简单的、基于多线程的、可配置的Web Server，支持静态文件访问，以及php文件的访问（简单实现）。

## 编译
在项目中事先写好了一个脚本用于执行编译：
```./build.sh```
编译完成后，会在bin目录下生成一个可执行文件tws

也可以手工执行编译：
```gcc -o bin/tws -lrt src/tws.c src/base.c```

## 配置服务器
服务器的配置文件在conf目录下，一个具体的示例如下：
```
[globals]
\# port number
port = 88

[dirs]
\# website dir
www = /vagrant/www/github/tws/demo/public

\# log dir
log = /vagrant/www/github/tws/demo/log


[extensions]
php = /usr/bin/php
```

其中： 

**port** - 指定服务器将要监听的端口（一般为80）； 

**www**  - 指向web项目的入口； 

**log**  - 指定生成日志的目录（当发生错误时，会按日期记录日志）； 

**php**  - 指定php位置（用于以`命令行方式`执行php文件）； 


## 启动服务器
```./bin/tws```


之后就可以访问服务器上的资源了，根据如上对demo项目的配置，可以访问项目首页：
http://192.168.1.2:88/index.html
如下：
![image](https://github.com/woojean/tws/blob/master/images/html.png)

