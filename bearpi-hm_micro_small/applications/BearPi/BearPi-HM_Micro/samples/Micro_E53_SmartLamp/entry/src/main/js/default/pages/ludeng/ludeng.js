/*
 * 智慧台灯 HAP 应用逻辑 - Kolton 2026-06-11
 * 功能：开关灯、亮度调节、场景模式、定时关灯
 */
import router from '@system.router';
import app from '@system.app';

/* 命令码定义，与驱动层一致 */
var cmdno = {
    init: 0,
    close: 1,
    read: 2,
    deng: 3,
    brightness: 4   /* 新增：亮度设置命令 */
};

/* 场景模式亮度预设 */
var scenePresets = {
    read: 100,      /* 阅读模式 */
    relax: 60,      /* 休闲模式 */
    night: 10,      /* 夜灯模式 */
    custom: 50      /* 自定义默认值 */
};

export default {
    data: {
        mydata: {
            Lux: 0,
            LED: 'OFF',
            Brightness: 0
        },
        interval: 0,
        currentScene: '',           /* 当前场景 */
        customBrightness: 50,       /* 自定义亮度 */
        selectedTimer: 0,           /* 选中的定时时间（分钟） */
        timerRemaining: 0,          /* 剩余时间（秒） */
        timerDisplay: '',           /* 显示的倒计时文本 */
        timerInterval: 0            /* 定时器计时器 */
    },

    onInit() {
        /* 初始化设备 */
        app.e53sc1service({
            cmd: cmdno.init,
            success(res) {},
            fail(res) {},
            complete(res) {}
        });
        /* 启动数据轮询，每2秒刷新一次 */
        this.interval = setInterval(() => this.queryData(), 2000);
    },

    onDestroy() {
        /* 清理所有定时器 */
        if (this.interval) {
            clearInterval(this.interval);
        }
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
        }
        /* 关闭设备 */
        app.e53sc1service({
            cmd: cmdno.close,
            success(res) {},
            fail(res) {},
            complete(res) {}
        });
    },

    /* 返回首页 */
    toback() {
        this.onDestroy();
        router.replace({
            uri: 'pages/index/index'
        });
    },

    /* 切换灯光开关 */
    toggleLight() {
        let that = this;
        let cmd = (this.mydata.LED === 'ON') ? 'OFF' : 'ON';
        app.e53sc1service({
            cmd: cmdno.deng,
            data: cmd,
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res) {},
            complete(res) {}
        });
    },

    /* 读取传感器数据 */
    queryData() {
        let that = this;
        app.e53sc1service({
            cmd: cmdno.read,
            data: '',
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res) {},
            complete(res) {}
        });
    },

    /* 滑动条亮度调节 */
    onBrightnessChange(e) {
        let that = this;
        let brightness = e.value;
        app.e53sc1service({
            cmd: cmdno.brightness,
            data: String(brightness),
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res) {},
            complete(res) {}
        });
    },

    /* 设置场景模式 */
    setScene(scene) {
        let that = this;
        let brightness = scenePresets[scene];
        if (scene === 'custom') {
            brightness = this.customBrightness;
        } else {
            this.customBrightness = this.mydata.Brightness; /* 切换前保存当前亮度为自定义 */
        }
        this.currentScene = scene;
        app.e53sc1service({
            cmd: cmdno.brightness,
            data: String(brightness),
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res) {},
            complete(res) {}
        });
    },

    /* 设置定时关灯 */
    setTimer(minutes) {
        let that = this;
        /* 清除之前的定时器 */
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
        }
        this.selectedTimer = minutes;
        this.timerRemaining = minutes * 60;
        this.updateTimerDisplay();

        /* 启动倒计时 */
        this.timerInterval = setInterval(() => {
            that.timerRemaining--;
            that.updateTimerDisplay();
            if (that.timerRemaining <= 0) {
                clearInterval(that.timerInterval);
                that.timerInterval = 0;
                that.selectedTimer = 0;
                /* 时间到，自动关灯 */
                that.autoOff();
            }
        }, 1000);
    },

    /* 取消定时 */
    cancelTimer() {
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = 0;
        }
        this.selectedTimer = 0;
        this.timerRemaining = 0;
        this.timerDisplay = '';
    },

    /* 更新定时显示文本 */
    updateTimerDisplay() {
        if (this.timerRemaining > 0) {
            let min = Math.floor(this.timerRemaining / 60);
            let sec = this.timerRemaining % 60;
            this.timerDisplay = (min < 10 ? '0' : '') + min + ':' + (sec < 10 ? '0' : '') + sec;
        } else {
            this.timerDisplay = '';
        }
    },

    /* 定时到自动关灯 */
    autoOff() {
        let that = this;
        app.e53sc1service({
            cmd: cmdno.deng,
            data: 'OFF',
            success(res) {
                that.mydata = JSON.parse(res.e53_sc1);
            },
            fail(res) {},
            complete(res) {}
        });
    }
}
