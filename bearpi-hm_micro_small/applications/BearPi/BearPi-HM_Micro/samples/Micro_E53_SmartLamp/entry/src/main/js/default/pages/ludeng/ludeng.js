import router from '@system.router';
import app from '@system.app';

var cmd = {
    init: 0,
    close: 1,
    read: 2,
    deng: 3,
    brightness: 4
};

var sceneBrightness = {
    read: 100,
    relax: 60,
    night: 10
};

export default {
    data: {
        mydata: {
            Lux: 0,
            LED: 'OFF',
            Brightness: 0
        },
        interval: 0,
        timerInterval: 0,
        currentScene: 'custom',
        customBrightness: 50,
        lastBrightness: 50,
        selectedTimer: 0,
        timerDelay: 600,
        timerRemaining: 0,
        timerDisplay: ''
    },

    onInit() {
        app.e53sc1service({
            cmd: cmd.init,
            success(res) {},
            fail(res) {},
            complete(res) {}
        });
        this.queryData();
        this.interval = setInterval(() => this.queryData(), 2000);
    },

    onDestroy() {
        if (this.interval) {
            clearInterval(this.interval);
            this.interval = 0;
        }
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = 0;
        }
        app.e53sc1service({
            cmd: cmd.close,
            success(res) {},
            fail(res) {},
            complete(res) {}
        });
    },

    toback() {
        this.onDestroy();
        router.replace({
            uri: 'pages/index/index'
        });
    },

    exit() {
        this.toback();
    },

    applyState(data) {
        this.mydata = data;
        var brightness = Number(data.Brightness);
        if (isNaN(brightness)) {
            brightness = 0;
        }
        if (brightness > 0) {
            this.lastBrightness = brightness;
        }
        if (data.LED !== 'ON') {
            this.currentScene = 'custom';
            if (brightness > 0) {
                this.customBrightness = brightness;
            }
            return;
        }
        if (brightness === sceneBrightness.read) {
            this.currentScene = 'read';
        } else if (brightness === sceneBrightness.relax) {
            this.currentScene = 'relax';
        } else if (brightness === sceneBrightness.night) {
            this.currentScene = 'night';
        } else {
            this.currentScene = 'custom';
            this.customBrightness = brightness;
        }
    },

    handleServiceResponse(res) {
        if (!res || !res.e53_sc1) {
            return;
        }
        var data = JSON.parse(res.e53_sc1);
        this.applyState(data);
    },

    sendPower(state) {
        var that = this;
        app.e53sc1service({
            cmd: cmd.deng,
            data: state,
            success(res) {
                that.handleServiceResponse(res);
            },
            fail(res, code) {},
            complete() {}
        });
    },

    sendBrightness(value) {
        var that = this;
        app.e53sc1service({
            cmd: cmd.brightness,
            data: String(value),
            success(res) {
                that.handleServiceResponse(res);
            },
            fail(res, code) {},
            complete() {}
        });
    },

    turnOn() {
        var brightness = Number(this.mydata.Brightness);
        if (isNaN(brightness) || brightness <= 0) {
            brightness = this.lastBrightness > 0 ? this.lastBrightness : 50;
            this.currentScene = 'custom';
            this.customBrightness = brightness;
            this.sendBrightness(brightness);
            return;
        }
        this.sendPower('ON');
    },

    turnOff() {
        this.sendPower('OFF');
    },

    toggleLight() {
        if (this.mydata.LED === 'ON') {
            this.sendPower('OFF');
            return;
        }
        this.turnOn();
    },

    queryData() {
        var that = this;
        app.e53sc1service({
            cmd: cmd.read,
            data: '',
            success(res) {
                that.handleServiceResponse(res);
            },
            fail(res, code) {},
            complete() {}
        });
    },

    onBrightnessChange(e) {
        var brightness = Number(e.value);
        if (isNaN(brightness)) {
            brightness = 0;
        }
        this.currentScene = 'custom';
        this.customBrightness = brightness;
        if (brightness > 0) {
            this.lastBrightness = brightness;
        }
        this.sendBrightness(brightness);
    },

    adjustBrightness(step) {
        var brightness = Number(this.mydata.Brightness);
        if (isNaN(brightness)) {
            brightness = 0;
        }
        brightness += step;
        if (brightness < 0) {
            brightness = 0;
        }
        if (brightness > 100) {
            brightness = 100;
        }
        this.currentScene = 'custom';
        this.customBrightness = brightness;
        if (brightness > 0) {
            this.lastBrightness = brightness;
        }
        this.sendBrightness(brightness);
    },

    brightnessMinus() {
        this.adjustBrightness(-5);
    },

    brightnessPlus() {
        this.adjustBrightness(5);
    },

    setScene(scene) {
        var brightness = sceneBrightness[scene];
        if (scene === 'custom') {
            brightness = this.customBrightness;
        } else {
            this.customBrightness = Number(this.mydata.Brightness);
        }
        this.currentScene = scene;
        this.sendBrightness(brightness);
    },

    sceneRead() {
        this.setScene('read');
    },

    sceneRelax() {
        this.setScene('relax');
    },

    sceneNight() {
        this.setScene('night');
    },

    sceneCustom() {
        this.setScene('custom');
    },

    setTimer(minutes) {
        var that = this;
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = 0;
        }
        if (minutes <= 0) {
            this.cancelTimer();
            return;
        }
        this.selectedTimer = minutes;
        this.timerDelay = minutes * 60;
        this.timerRemaining = minutes * 60;
        this.updateTimerDisplay();
        this.timerInterval = setInterval(() => {
            that.timerRemaining--;
            if (that.timerRemaining <= 0) {
                that.autoOff();
                that.cancelTimer();
                return;
            }
            that.updateTimerDisplay();
        }, 1000);
    },

    timer10() {
        this.setTimer(10);
    },

    timer30() {
        this.setTimer(30);
    },

    timer60() {
        this.setTimer(60);
    },

    changeTimerDelay(e) {
        var delay = Number(e.value);
        if (isNaN(delay)) {
            delay = 600;
        }
        if (delay < 10) {
            delay = 10;
        }
        if (delay > 3600) {
            delay = 3600;
        }
        this.timerDelay = delay;
        this.setTimerBySeconds(delay);
    },

    setTimerBySeconds(seconds) {
        var that = this;
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = 0;
        }
        if (seconds <= 0) {
            this.cancelTimer();
            return;
        }
        this.selectedTimer = Math.ceil(seconds / 60);
        this.timerDelay = seconds;
        this.timerRemaining = seconds;
        this.updateTimerDisplay();
        this.timerInterval = setInterval(() => {
            that.timerRemaining--;
            if (that.timerRemaining <= 0) {
                that.autoOff();
                that.cancelTimer();
                return;
            }
            that.updateTimerDisplay();
        }, 1000);
    },

    timerMinus() {
        var delay = this.timerDelay - 60;
        if (delay < 10) {
            delay = 10;
        }
        this.setTimerBySeconds(delay);
    },

    timerPlus() {
        var delay = this.timerDelay + 60;
        if (delay > 3600) {
            delay = 3600;
        }
        this.setTimerBySeconds(delay);
    },

    cancelTimer() {
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = 0;
        }
        this.selectedTimer = 0;
        this.timerRemaining = 0;
        this.timerDisplay = '';
    },

    updateTimerDisplay() {
        var min = Math.floor(this.timerRemaining / 60);
        var sec = this.timerRemaining % 60;
        this.timerDisplay = (min < 10 ? '0' : '') + min + ':' + (sec < 10 ? '0' : '') + sec;
    },

    autoOff() {
        this.sendPower('OFF');
    }
}
