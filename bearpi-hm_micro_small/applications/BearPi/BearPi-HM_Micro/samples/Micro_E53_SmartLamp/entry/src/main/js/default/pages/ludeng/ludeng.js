import router from '@system.router';
import app from '@system.app';

var cmdno = {init:0, close:1, read:2, deng:3, brightness:4};

export default {
    data: {
        mydata: {
            Lux: 0,
            LED: 'OFF',
            Brightness: 0
        },
        interval: 0
    },

    onInit() {
        app.e53sc1service({
            cmd: cmdno.init,
            success(res){},
            fail(res){},
            complete(res){}
        });
        this.interval = setInterval(() => this.queryData(), 2000);
    },

    onDestroy() {
        if (this.interval) {
            clearInterval(this.interval);
        }
        app.e53sc1service({
            cmd: cmdno.close,
            success(res){},
            fail(res){},
            complete(res){}
        });
    },

    toggleLight() {
        let that = this;
        let cmd = (this.mydata.LED === 'ON') ? 'OFF' : 'ON';
        app.e53sc1service({
            cmd: cmdno.deng,
            data: cmd,
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res){},
            complete(res){}
        });
    },

    queryData() {
        let that = this;
        app.e53sc1service({
            cmd: cmdno.read,
            data: '',
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res){},
            complete(res){}
        });
    }
}
