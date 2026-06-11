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
import app from '@system.app';
import router from '@system.router';

export default {
    data: {
        title: 'World',
        e53: {
            e53_nong: {
                state: '农业', uri: 'pages/nongye/nongye', ico:'../../common/case_nong.png'
            },
            e53_deng: {
                state: '路灯', uri: 'pages/ludeng/ludeng', ico:'../../common/case_deng.png'
            },
            e53_yan: {
                state: '烟感', uri: 'pages/yangan/yangan', ico:'../../common/case_yan.png'
            },
            e53_jing: {
                state: '井盖', uri: 'pages/jinggai/jinggai', ico:'../../common/case_jing.png'
            },
            e53_hong: {
                state: '红外', uri: 'pages/hongwai/hongwai', ico:'../../common/case_hong.png'
            },
        }
    },
    exit(){
        app.terminate()
    },
    route(e){
        router.replace({
            uri: e.uri
        });
    }
}
