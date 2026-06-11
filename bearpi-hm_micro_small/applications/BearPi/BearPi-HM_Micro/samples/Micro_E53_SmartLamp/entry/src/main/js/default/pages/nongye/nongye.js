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

var cmdno = {init:0,close:1,read:2,dian:3,deng:4}
export default {
    data: {
        title: '智慧农业',
        mydata: {
            Temp: 0,
            Hum: 0,
            Lux: 0,
            Motor: 'OFF',
            LED: 'OFF'
        },
        interval:0
    },
    onInit(){
        app.e53ia1service({
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
        app.e53ia1service({
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
    dengChange(){
        let that = this
        let flag = ''
        if(this.mydata.LED == 'OFF'){
            flag = 'ON'
        }
        else{
            flag = 'OFF'
        }
        app.e53ia1service({
            cmd:cmdno.deng,
            data:flag,
            success(res){
                that.mydata = JSON.parse(res.e53_ia1)
            },
            fail(res){

            },
            complete(res){

            },
        })
    },
    dianChange(){
        let that = this
        let flag = ''
        if(this.mydata.Motor == 'OFF'){
            flag = 'ON'
        }
        else{
            flag = 'OFF'
        }
        app.e53ia1service({
            cmd:cmdno.dian,
            data:flag,
            success(res){
                that.mydata = JSON.parse(res.e53_ia1)
            },
            fail(res){

            },
            complete(res){

            },
        })
    },
    queryData(){
        let that = this
        app.e53ia1service({
            cmd:cmdno.read,
            data:'',
            success(res){
                that.mydata = JSON.parse(res.e53_ia1)
            },
            fail(res){

            },
            complete(res){

            },
        })
    }
}
