# getdepth

Kinect v1, Kinect v2, DepthSense DS325/311, RealSense D415/D435 からカラーとデプスを取得するサンプル

    Copyright (c) 2011-2019 Kohe Tokoi. All Rights Reserved.
    
    Permission is hereby granted, free of charge,  to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction,  including without limitation the rights
    to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell
    copies or substantial portions of the Software.
    
    The above  copyright notice  and this permission notice  shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
    IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
    FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL
    KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN
    AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## クラスについて

### KinectV1 / KinectV2 / Ds325 / Rs400 クラスについて

* KinectV1 クラスは Kinect for Windows SDK 1.8 を使います。
* KinectV2 クラスは Kinect for Windows SDK 2.0 を使います。
* Ds325 クラスは DepthSense SDK 1.9 を使います。
* Rs400 クラスは librealsense2 を使います。
* 取得したカラーとデプスは OpenGL のテクスチャに格納します。
* カラーのサンプリングに使うテクスチャ座標も計算します。

### KinectV1 クラスの使い方

* KinectV1 クラスのオブジェクトを作ってください。
* 一応、マルチセンサに対応してみましたが、テストしていません。
* getActivated() メソッドは使用されている Kinect の数を返します。
* Kinect の起動に失敗していれば例外を投げます。

### KinectV2 クラスの使い方

* KinectV2 クラスのオブジェクトを作ってください。
* Kinect v2 は一台の PC につき一台しか使えません。
* getActivated() メソッドは Kinect v2 が使えれば 1 を返します。
* Kinect の起動に失敗していれば例外を投げます。

### Ds325 クラスの使い方

* Ds325 クラスのオブジェクトを作ってください。
* 一応、マルチセンサに対応してみましたが、テストしていません。
* getActivated() メソッドは使用されている DepthSense の数を返します。
* DepthSense が 1 台も起動できなければ例外を投げます。

### Rs400 クラスの使い方

* Ds325 クラスのオブジェクトを作ってください。
* 一応、マルチセンサに対応しています (3 台同時キャプチャの実績あり)。
* getActivated() メソッドは使用されている RealSense の数を返します。
* RealSense が 1 台も起動できなければ例外を投げます。

### 共通の設定

* getDepth() メソッドを呼ぶとデプスをテクスチャに転送し、そのテクスチャを bind します。
* この時、同時にカラーのサンプリングに使うテクスチャ座標も計算します (Rs400 を除く)。
* getPoint() メソッドはカメラ座標をテクスチャに、テクスチャ座標をバッファオブジェクトに転送します。
* getPosition() メソッドは getPoint() メソッドを GPU 実装したものです。
* Rs400 クラスではテクスチャ座標を getPoint() および getPosition() メソッドで計算します。
* getPoint() あるいは getPosition() メソッドで作成したテクスチャを VTF で頂点座標に使ってください。 
* getColor() メソッドはカラーをテクスチャに転送し、そのテクスチャを bind します。
* getUvmapBuffer() メソッドはテクスチャ座標の格納先のバッファオブジェクトを返します。
* これを描画する VAO に組み込んで getColor() メソッドで得たカラーデータをマッピングしてください。
* とにかく getdepth.cpp を読んでください。

## サンプルプログラムについて

### サンプルプログラムの概要

* OpenGL のテクスチャに入っているデプスマップを使ってポリゴンメッシュを描きます。
* シェーダを使ってテクスチャに入っているデプスからポイントの座標を求めてテクスチャに格納します。
* position_xx.comp で作ったテクスチャから normal.comp を使って法線ベクトルを求めています。
* Kinect V1 / V2 版では NuiTransformDepthImageToSkeleton() 相当の計算を position_v1(v2).comp で行っています。
* RealSense 版では getPoint() で取得したテクスチャから normal.frag を使って法線ベクトルを求めています。
* この二つのテクスチャとカラーのテクスチャを使ってメッシュをレンダリングしています。
* 頂点属性はテプスとカラーのテクスチャをサンプリングするテクスチャ座標だけを送っています。
* simple.frag の main() の内容を変更してみてください。

  * fc = idiff + ispec;

    * グレーに陰影がついたものになります。
    * マテリアルは main() の material で設定しています。

  * fc = texture(color, texcoord);

    * メッシュにカラーがマッピングされます。

  * fc = texture(color, texcoord) * idiff + ispec;

    * メッシュにカラーをマッピングして、さらに正面から光を当てます。
    * 陰影をつけたものに陰影を重ねてつけるので、かなり変になります。

### サンプルプログラムの操作方法

* マウスの左ドラッグで視点を上下左右に移動できます。
* マウスの右ドラッグで視点の向きを変更できます。
* マスのホイールで向いている方向に前後できます。
* ESC で終了します。

## その他

* このプログラムは学生さんに説明するために書き始めたものですので、実用的ではありません。
* テプスを頂点属性ではなくテクスチャにしたのは GPU によるフィルタリングの実験に使うためです。
* 計測不能点は discard してもよかったんですが、これもある目的のために遠方に飛ばしています。
