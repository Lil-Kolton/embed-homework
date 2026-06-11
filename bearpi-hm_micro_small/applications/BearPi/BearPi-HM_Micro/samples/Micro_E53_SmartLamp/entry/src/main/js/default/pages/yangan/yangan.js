/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import router from '@system.router';
import app from '@system.app';

var cmdno = {init:0,close:1,read:2,beep:3}
export default {
    data: {
        mydata: {
            ppm: 0,
            Beep: 'OFF'
        },
        threshold: 300,
        interval: 0
    },
    onInit(){
        app.e53sf1service({
            cmd:cmdno.init,
            success(res){
            },
            fail(res){

            },
            complete(res){

            },
        })
        this.interval = setInterval(()=>this.queryData(),2000)
    },
    toback(){
        app.e53sf1service({
            cmd:cmdno.close,
            success(res){
            },
            fail(res){

            },
            complete(res){

            },
        })
        clearInterval(this.interval)
        router.replace({
            uri: 'pages/index/index'
        });
    },
    thresholdChange(e){
        this.threshold = e.value
    },
    openAlert(){
        app.e53sf1service({
            cmd:cmdno.beep,
            data:'ON',
            success(res){

            },
            fail(res){

            },
            complete(res){

            },
        })
    },
    closeAlert(){
        app.e53sf1service({
            cmd:cmdno.beep,
            data:'OFF',
            success(res){

            },
            fail(res){

            },
            complete(res){

            },
        })
    },
    queryData(){
        let that = this
        app.e53sf1service({
            cmd:cmdno.read,
            data:'',
            success(res){
                that.mydata = JSON.parse(res.e53_sf1)
                if(that.mydata.ppm > that.threshold && that.mydata.Beep == 'OFF'){
                    that.openAlert()
                }
                else if(that.mydata.ppm < that.threshold && that.mydata.Beep == 'ON'){
                    that.closeAlert()
                }
                else{

                }
            },
            fail(res){

            },
            complete(res){

            },
        })
    }
}
