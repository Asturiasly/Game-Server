# Game-Server
# Реализация браузерной игры протестировано и поддерживается на базе Ubuntu LTS 20.04.

# Предварительные установки для системы, где необходимо развернуть программу:
apt update && \
   apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# С помощью пакетного менеджера conan версии <2 происходит установка нужных зависимостей под Linux:
 conan install .. --build=missing -s build_type=Debug
 conan install .. --build=missing -s build_type=Release
 conan install .. --build=missing -s build_type=RelWithDebInfo
 conan install .. --build=missing -s build_type=MinSizeRel
# Под Windows: 
 conan install .. --build=missing -s build_type=Debug -s compiler.runtime=MD
 conan install .. --build=missing -s build_type=Release -s compiler.runtime=MT
 conan install .. --build=missing -s build_type=RelWithDebInfo -s compiler.runtime=MT
 conan install .. --build=missing -s build_type=MinSizeRel -s compiler.runtime=MT

# Далее с помощью CMake происходит компоновка и компиляция:
 cmake .. -DCMAKE_BUILD_TYPE=Debug .. && \
 cmake --build .
