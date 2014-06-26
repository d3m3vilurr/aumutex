var cluster = require('cluster');
var sleep = require('sleep');
var aumutex = require('./aumutex');

var mutex_name = '/var/tmp/mymutex';

if (cluster.isMaster) {

    cluster.fork();
    cluster.on('exit', function(worker, code, signal) {
        console.log('worker ' + worker.process.pid + ' died');
    });
    sleep.sleep(2);

    var master_mutex = aumutex.create(mutex_name);
    console.info('[MASTER] create');
    
    console.info('[MASTER] wait');
    aumutex.enter(master_mutex);
    console.info('[MASTER] enter');
    
    console.info('[MASTER] close');
    aumutex.close(master_mutex);
    
    master_mutex = aumutex.create(mutex_name);
    console.info('[MASTER] re-create');
    
    console.info('[MASTER] wait');
    sleep.sleep(2);
    aumutex.enter(master_mutex);
    console.info('[MASTER] re-enter');
    
    aumutex.close(master_mutex);
    console.info('[MASTER] process.exit()');
    process.exit();

} else {

    var worker_mutex = aumutex.create(mutex_name);
    console.info('[WORKER] create');
    
    aumutex.enter(worker_mutex);
    console.info('[WORKER] enter');
    
    sleep.sleep(2);
    console.info('[WORKER] leave');
    aumutex.leave(worker_mutex);
    
    sleep.sleep(2);
    console.info('[WORKER] wait');
    aumutex.enter(worker_mutex);
    console.info('[WORKER] re-enter');
    sleep.sleep(2);
    
    console.info('[WORKER] process.exit()');
    process.exit();
}