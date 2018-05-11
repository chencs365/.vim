#!/bin/bash
git clone https://github.com/chencs365/.vim.git
ln -s ~/.vim/vimrc ~/.vimrc
cd ~/.vim
git submodule init
git submodule update
