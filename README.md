An interactive [`Procfile`](https://ddollar.github.io/foreman/#PROCFILE) runner, meant as a replacement for [`foreman`](http://ddollar.github.io/foreman/) when ran locally. 

Main differences from `foreman`:

* `stdout` and `stderr` are written off-screen, and can be viewed interactively when you want to observe the log output.
* `stdout` and `stderr` viewing will use whatever is defined in `PAGER`, or `more` by default.
* Processes by default are launched, and then kept running. If a process dies, it is restarted. If you want to stop a process, you tell `fiveman` that you want it stopped. 

Example:

![fiveman in use](https://raw.github.com/netshade/fiveman/master/example.gif)


