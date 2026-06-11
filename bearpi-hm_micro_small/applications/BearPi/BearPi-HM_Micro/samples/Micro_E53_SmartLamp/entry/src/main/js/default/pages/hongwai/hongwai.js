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

var cmdno = {init:0,close:1,read:2}
export default {
    data: {
        mydata: {
            humanAround: 0
        },
        state:{
            0:'无人经过',
            1:'有人经过'
        },
        interval: 0
    },
    onInit(){
        app.e53is1service({
            cmd:cmdno.init,
            success(res){
            },
            fail(res){

            },
            complete(res){

            },
        })
        this.interval = setInterval(()=>this.queryData(),500)
    },
    toback(){
        app.e53is1service({
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
    queryData(){
        let that = this
        app.e53is1service({
            cmd:cmdno.read,
            data:'',
            success(res){
                that.mydata = JSON.parse(res.e53_is1)
            },
            fail(res){

            },
            complete(res){

            },
        })
    }
}
