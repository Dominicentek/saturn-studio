FROM debian:buster

SHELL ["/bin/bash", "-c"]

# debian buster core dependencies - managed by apt
COPY <<-"eot" /etc/apt/sources.list 
deb http://cdn-fastly.deb.debian.org/debian/ buster main contrib
deb-src http://cdn-fastly.deb.debian.org/debian/ buster main contrib
deb http://cdn-fastly.deb.debian.org/debian/ buster-updates main contrib
deb-src http://cdn-fastly.deb.debian.org/debian/ buster-updates main contrib
deb http://security.debian.org/ buster/updates main contrib
deb-src http://security.debian.org/ buster/updates main contrib
eot
RUN apt update
RUN apt upgrade -y
RUN apt build-dep -y libsdl2-dev libgtk-3-dev
RUN apt install -y git python3 python3-pip wget gpg libgbm-dev libglew-dev ninja-build

# meson - from this point on, custom builds
RUN python3 -m pip install --upgrade meson

# gtk4
RUN <<eot
source /etc/profile
wget https://download.gnome.org/sources/gtk/4.13/gtk-4.13.9.tar.xz
wget https://download.gnome.org/sources/gtk/4.13/gtk-4.13.9.sha256sum
if ! sha256sum --check --status gtk-*.sha256sum; then
    echo 'failed to verify downloaded GTK4'
    exit 1
fi
tar xf gtk-*.tar.xz
cd gtk-*/
meson subprojects download
cd subprojects/glib/
patch << 'EOF'
diff --git a/meson_options.txt b/meson_options.txt
index 517d575..fc6acd8 100644
--- a/meson_options.txt
+++ b/meson_options.txt
@@ -79,7 +79,7 @@ option('force_posix_threads',

 option('tests',
        type : 'boolean',
-       value : true,
+       value : false,
        description : 'build tests')

 option('installed_tests',
EOF
cd -
meson setup --prefix=/usr build -Dwayland-backend=false -Dvulkan=disabled -Dmedia-gstreamer=disabled -Dbuild-tests=false -Dbuild-examples=false -Dbuild-testsuite=false -Dbuild-demos=false -Ddemos=false
meson compile -Cbuild
meson install -Cbuild
eot

# sdl2
RUN <<eot
wget https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-2.30.1.tar.gz
wget https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-2.30.1.tar.gz.sig
gpg --keyserver hkps://pgp.surf.nl --recv-key 30A59377A7763BE6 
if ! gpg --verify SDL2-*.tar.gz.sig SDL2-*.tar.gz; then
    echo 'failed to verify downloaded SDL2'
    exit 1
fi
tar xf SDL2-*.tar.gz
cd SDL2-*/
mkdir build
cd build
../configure --prefix=/usr --disable-video-vulkan --disable-video-wayland
make -j$(nproc)
make install
eot

RUN <<eot
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
./linuxdeploy-x86_64.AppImage --appimage-extract
cd squashfs-root/usr/bin/
wget https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh
chmod +x linuxdeploy-plugin-gtk.sh
eot

RUN mkdir /saturn-studio
RUN git config --global --add safe.directory /saturn-studio
WORKDIR /saturn-studio