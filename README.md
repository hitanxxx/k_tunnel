# K_TUNNEL
K-tunnel是一个SSL tunnel(翻墙模式)，同时也是一个http proxy。全部代码由c编写。</br>
当前版本只支持linux系统。</br>
双向SSL加密。安全性高。信息只能在ip层面进行跟踪。应用流量只能阻隔，无法篡改，跟踪。</br>
采用事件驱动，多进程模式。包含完整的信号管理以及进程管理，日志。</br>
性能十分的不错，同时对硬件资源的要求十分的低。适合各种低性能的硬件环境。SSL tunnel client 模式工作在1G DDR2 RAM的树莓派2上，正常的使用程度。cpu与内存的使用率基本维持维持在2%。
![example](./google.png)
# Install
依赖OpenSSL库。确保有OpenSSL库之后，到根目录运行。
* configure
* make && make install </br>
安装完成后，在/usr/local/ktunnel 目录可看到安装完成后的文件。
运行/usr/local/ktunnel/sbin目录下的elf文件即可使用，但是使用之前可能需要了解配置。
> * /usr/local/ktunnel/conf - 配置文件目录。
> * /usr/local/ktunnel/logs - pid，日志文件目录。
> * /usr/local/ktunnel/sbin - elf执行文件所在目录。
> * /usr/local/ktunnel/certificate  - ssl文件目录
# Command line parameters
* -stop </br>
作用是停止后台所有进程。</br>
stop all process when works in the backend
* -reload </br>
作用是重新启动子进程 </br>
reload all worker process

# configuraction
```json
{
	"daemon":true,
	"worker_process":2,

	"reuse_port":false,
	"accept_mutex":false,

	"log_error":true,
	"log_debug":false,

	"sslcrt":"/usr/local/ktunnel/certificate/server.crt",
	"sslkey":"/usr/local/ktunnel/certificate/server.key",

	"tunnel":{
		"mode":"single"
	}
}
```
一个典型的配置文件如上，结构划分为两个个部分：
* 全局部分。</br>
一般设置一些特征信息</br>
> * daemon - 守护进程开关
> * worker_process - 工作进程数量，为0时，管理进程即为工作进程。
> * reuse_port - socket特性，某些情景优化多进程竞争态。
> * accept_mutex - 信号量锁开关。
> * log_error - error日志开关。
> * log_debug - debug日志开关。（影响性能）
> * sslcrt - SSL证书&公钥。
> * sslkey - SSL私钥。

* tunnel部分。</br>
设置一些挂于tunnel的信息。</br>
> * mode - tunnel工作的模式，支持 single，server，client三种模式。（single为proxy模式，而client与server模式则需要配合使用，组成SSL tunnel加密通道）
> * serverip - 当mode指定为client的时候，需要额外的指定一个serverip。以告诉程序server的地址。

# Tips
* single模式使用7325端口。
> * browser ------- single ---------- Internet

* client模式使用的端口与single一样也是7325。通常情况浏览器的代理信息设置到7325端口。
* server使用7324端口。server的7324端口与client的任意端口形成SSL tunnel加密通道。以确保传输的信息的安全。
> * browser ------- SSL client ======= SSL server ----- Internet
