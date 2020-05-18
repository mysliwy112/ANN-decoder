#include<iostream>

#include "am_utils.h"
#include "PNGformat.h"
#include "filesys.h"
#include "CLZW.h"
#include "CRLE.h"

#include "Image.h"

using namespace std;


namespace am{
    Image::Image()
    {
        //ctor
    }

    void Image::load_ann(bytes::iterator &offset)
    {
        width=get_int(offset,0x2);
        height=get_int(offset,0x2);
        position_x=get_int(offset,0x2);
        position_y=get_int(offset,0x2);
        compression=get_int(offset,0x2);
        image_size=get_int(offset,0x4);

        advance(offset,0x4);
        advance(offset,0xA);

        alpha_size=get_int(offset,0x4);

        name=get_str(offset,0x14);

        if(log){
            cout<<"Image: "<<name<<endl;
            cout<<"x:"<<position_x<<" y:"<<position_y<<endl;
            cout<<"width:"<<width<<" height:"<<height<<endl;
            cout<<"Compression: "<<compression<<endl;
            cout<<"image_size: "<<image_size<<endl;
            cout<<"alpha_size: "<<alpha_size<<endl;
            cout<<endl;

        }
    }

    void Image::load_img(bytes::iterator &offset)
    {
        if(get_str(offset,0x4)=="PIK"){
            cout<<"This is img."<<endl;
        }
        width=get_int(offset,0x4);
        height=get_int(offset,0x4);
        bpp=get_int(offset,0x4);
        image_size=get_int(offset,0x4);

        advance(offset,0x4);

        int comp=get_int(offset,0x4);
        if(comp==4)
            comp=0;
        compression=comp;
        alpha_size=get_int(offset,0x4);
        position_x=get_int(offset,0x4);
        position_y=get_int(offset,0x4);
    }

    void Image::load_data(bytes data)
    {
        image_data img;
        if(data.size()==image_size+alpha_size){
            img.image.assign(data.begin(),data.begin()+image_size);
            img.alpha.assign(data.begin()+image_size,data.end());
        }
        img.image=decompress(img.image,compression,2);
        img.alpha=decompress(img.alpha,compression,1);
        compression=0;
        create_rgba32(img);
    }

    void Image::load_rgba32(bytes data){
        rgba32=data;
    }

    bytes Image::get_ann_header(int compression){
        bytes data(0x34);
        bytes::iterator offset=data.begin();

        set_int(offset,width,0x4);
        set_int(offset,height,0x4);

        set_int(offset,position_x,0x4);
        set_int(offset,position_y,0x4);

        set_int(offset,compression,0x4);

        set_int(offset,rgba32.size()/2,0x4);

        set_data(offset,bytes(1,4));
        set_data(offset,bytes(3,0));
        set_data(offset,bytes(0xE,0));

        set_int(offset,rgba32.size()/4,0x4);
        set_str(offset,name,0x14);

        return data;

    }

    bytes Image::get_img_header(int compression){
        bytes data(0x28);
        bytes::iterator offset=data.begin();

        set_str(offset,"PIK");
        set_data(offset,bytes(1,0));

        set_int(offset,width,0x4);
        set_int(offset,height,0x4);

        set_int(offset,bpp,0x4);
        set_int(offset,image_size,0x4);
        set_data(offset,bytes(4,0));
        set_int(offset,compression,0x4);
        set_int(offset,alpha_size,0x4);

        set_int(offset,position_x,0x4);
        set_int(offset,position_y,0x4);

        return data;
    }

    bytes Image::get_am_data(int compression){

        image_data img;
        img=split_rgba32();

        img.image=compress(img.image,compression,2);
        image_size=img.image.size();
        img.alpha=compress(img.alpha,compression,1);
        alpha_size=img.alpha.size();

        img.image.insert(img.image.end(),img.alpha.begin(),img.alpha.end());
        return img.image;
    }

    bytes Image::get_rgba32(){
        return rgba32;
    }

    void Image::read_png(string filename){
        Image image=read_PNG(filename);
        width=image.width;
        height=image.height;
        bpp=16;
        rgba32=image.rgba32;
    }

    void Image::write_png(string filename){
        write_PNG(rgba32,width,height,8,filename);
    }

    void Image::read_img(string filename){
        bytes data=read_file(filename);
        bytes::iterator i=data.begin();
        load_img(get_data(i,0x28));
        load_data(get_data(i,image_size+alpha_size));
    }

    void Image::write_img(string filename){
        int cmpr=2;
        bytes img=get_am_data(cmpr);
        bytes data=get_img_header(cmpr);

        data.insert(data.end(),img.begin(),img.end());
        //write_png(filename+".png");
        //data.insert(data.end(),rgba32.begin(),rgba32.end());
        write_file(filename,data);
    }

    bytes Image::decompress(bytes data, int type, int size){
        if(data.size()>0){
            try{
                if(type==0){
                }else if(type==4){
                        data=decodeCRLE(data,size);

                }else if(type==3){
                        data=decodeCLZW(data);
                        data=decodeCRLE(data,size);

                }else if(type==2){
                        data=decodeCLZW(data);

                }else{
                    throw invalid_argument(string("Unknown compression: ")+to_string(type));
                }
            }catch(...){
                cout<<"Can't decompress image, compression type:"<<type<<endl;
                throw"File decompress problem";
            }
        }

        return data;
    }

    bytes Image::compress(bytes data, int type, int size){
        if(data.size()>0){
            try{
                if(type==0){
                }else if(type==4){
                    data=codeCRLE(data,size);

                }else if(type==3){
                    data=codeCRLE(data,size);
                    data=codeCLZW(data);

                }else if(type==2){
                    data=codeCLZW(data);

                }else{
                    throw invalid_argument(string("Unknown compression: ")+to_string(type));
                }
            }catch(...){
                cout<<"Can't compress image, compression type:"<<type<<endl;
                throw"File decompress problem";
            }
        }


        return data;
    }

    void Image::align(){
        if(log)
            cout<<"Aligning... "<<position_x<<" "<<position_y<<" ";
        width=width+position_x;
        unsigned long long add=position_y*width*4;
        rgba32.insert(rgba32.begin(),add,0);
        unsigned long long pos=add;
        while(pos<rgba32.size()){
            add=4*position_x;
            rgba32.insert(rgba32.begin()+pos,add,0);
            pos+=width*4;
        }
        height=position_y+height;
        position_x=0;
        position_y=0;
        if(log)
            cout<<"completed"<<endl;
    }


    void Image::create_rgba32(image_data img){
        if(img.alpha.size()==0)
            img.alpha.resize(img.image.size()/3,255);

        bytes n(img.image.size()/2*3);
        int temp;
        int counter=0;
        for(unsigned int i=0;i<img.image.size();i+=2){
                temp=img.image[i]+img.image[i+1]*256;
                n[counter+2]=(temp%32)*256/32;
                temp=(temp-temp%32)/32;
                n[counter+1]=(temp%64)*256/64;
                temp=(temp-temp%64)/64;
                n[counter]=(temp%32)*256/32;
                counter+=3;
        }
        img.image=n;

        rgba32.resize(img.image.size()+img.alpha.size(),0);
        int al=0;
        int da=0;
        for(unsigned int i=0;i<rgba32.size();i+=4){
            rgba32[i]=img.image[da];
            rgba32[i+1]=img.image[da+1];
            rgba32[i+2]=img.image[da+2];
            rgba32[i+3]=img.alpha[al];
            da+=3;
            al++;
        }
    }

    image_data Image::split_rgba32(){
        image_data img;
        img.image.resize(rgba32.size()/2,0);
        img.alpha.resize(rgba32.size()/4,0);
        int im_p=0;
        int al_p=0;
        for(int i=0;i<rgba32.size();i+=4){
            img.image[im_p]+=uint8_t(float(rgba32[i+2])*32/256);
            img.image[im_p]+=uint8_t(float(rgba32[i+1])*64/256)<<5;
            img.image[im_p+1]+=uint8_t(float(rgba32[i+1])*64/256)>>3;
            img.image[im_p+1]+=uint8_t(float(rgba32[i])*32/256)<<3;
            img.alpha[al_p]=rgba32[i+3];
            im_p+=2;
            al_p+=1;
        }

        return img;
    }

    void Image::load_img(bytes data){
        bytes::iterator offset=data.begin();
        load_img(offset);
    }

};
