#!/bin/sh
homedir=$PWD
echo "hoemdir : ${homedir}"
install_dir=${homedir}/dist
libdir=${install_dir}/lib
include_dir=${install_dir}/include
include_raptor=${include_dir}/raptor2
include_rasqal=${include_dir}/rasqal

#determine 64 or 32
sys_dir="x86"
platform=`uname -i`
echo "platform : ${platform}"
if [ "${platform}" = "x86_64" ]; then
        sys_dir="x64"
fi


#define make function
make_install()
{
        make;
        make install;
}
xml_pcfile_old=${homedir}/libxml2/${sys_dir}/lib/pkgconfig/libxml-2.0.pc
xml_pcfile_new=${homedir}/libxml2/${sys_dir}/lib/pkgconfig/libxml-2.0.pc1
sed "1c\prefix=${homedir}/libxml2/${sys_dir}" ${xml_pcfile_old} > ${homedir}/libxml2/${sys_dir}/lib/pkgconfig/libxml-2.0.pc1 
mv ${xml_pcfile_new} ${xml_pcfile_old}

xml2_pkg_path=${homedir}/libxml2/${sys_dir}/lib/pkgconfig

#raptor2
cd ./raptor2
xml2_config=${homedir}/libxml2/${sys_dir}/bin/xml2-config
chmod +x configure
echo "./configure --with-xml2-config=${xml2_config} --prefix=${install_dir}"
./configure --with-xml2-config=${xml2_config} --prefix=${install_dir} 
make_install

#rasqal
raptor_config=${install_dir}/lib/pkgconfig
cd ${homedir}/rasqal
chmod +x configure
./configure  PKG_CONFIG_PATH=${xml2_pkg_path}:${raptor_config} --prefix=${install_dir}
export PKG_CONFIG_PATH=${libdir}/pkgconfig
make_install

#redland
dist_config=${install_dir}/lib/pkgconfig
cd ${homedir}/redland
chmod +x configure
./configure PKG_CONFIG_PATH=${xml2_pkg_path}:${dist_config} --prefix=${install_dir}
make_install

