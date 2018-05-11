" Basic Setting Begin---------------------------------------------
set nocompatible
filetype on
set nu
set encoding=utf-8
let mapleader = "\<Space>"
nnoremap <Leader>w :w<CR>
nnoremap <Leader>q :q<CR>
nnoremap <Leader>h <C-W>h
nnoremap <Leader>j <C-W>j
nnoremap <Leader>k <C-W>k
nnoremap <Leader>l <C-W>l
nnoremap <C-H> <C-W><
nnoremap <C-L> <C-W>>
nnoremap <C-J> <C-W>-
nnoremap <C-K> <C-W>+
nnoremap <s-tab> :bn<CR>
nnoremap <tab> :tabn<CR>
"set mouse=a
set cursorline 
"set cursorcolumn
set ruler
set ignorecase smartcase
set tabstop=4
set expandtab
set shiftwidth=4
set incsearch
set hlsearch
set smartindent
set backspace=indent,eol,start
set clipboard+=unnamedplus
highlight Search ctermbg=grey ctermfg=black
" Basic Setting End---------------------------------------------

filetype off
runtime bundle/vim-pathogen/autoload/pathogen.vim
execute pathogen#infect()
filetype plugin indent on

" Color scheme--------------------------------------------------
syntax enable
set background=dark
let g:solarized_termcolors=16
let g:solarized_termtrans=1
colorscheme solarized
" Setting NERDTree Begin----------------------------------------
let g:NERDTreeWinSize = 25
autocmd StdinReadPre * let s:std_in=1
autocmd VimEnter * if argc() == 0 && !exists("s:std_in") | NERDTree | endif
autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTree") && b:NERDTree.isTabTree()) | q | endif
let g:NERDTreeDirArrowExpandable = '▸'
let g:NERDTreeDirArrowCollapsible = '▾'
" this part is for nerdtree tabs
map <F2> <plug>NERDTreeTabsToggle<CR>
"let g:nerdtree_tabs_open_on_console_startup = 1
let g:NERDTreeIndicatorMapCustom = {
    \ "Modified"  : "✹",
    \ "Staged"    : "✚",
    \ "Untracked" : "✭",
    \ "Renamed"   : "➜",
    \ "Unmerged"  : "═",
    \ "Deleted"   : "✖",
    \ "Dirty"     : "✗",
    \ "Clean"     : "✔︎",
    \ 'Ignored'   : '☒',
    \ "Unknown"   : "?"
    \ }
" Setting NERDTree End------------------------------------------
"" Setting Vim-go Begin------------------------------------------
"au FileType go nmap <leader>r <Plug>(go-run)
"au FileType go nmap <leader>b <Plug>(go-build)
"au FileType go nmap <leader>t <Plug>(go-test)
"au FileType go nmap <leader>c <Plug>(go-coverage)
"au FileType go nmap <Leader>ds <Plug>(go-def-split)
"au FileType go nmap <Leader>dv <Plug>(go-def-vertical)
"au FileType go nmap <Leader>dt <Plug>(go-def-tab)
"au FileType go nmap <Leader>gd <Plug>(go-doc)
"au FileType go nmap <Leader>gv <Plug>(go-doc-vertical)
"au FileType go nmap <Leader>s <Plug>(go-implements)
"au FileType go nmap <Leader>d <Plug>(go-info)
"let g:go_highlight_functions = 1
"let g:go_highlight_methods = 1
"let g:go_highlight_fields = 1
"let g:go_highlight_types = 1
"let g:go_highlight_operators = 1
"let g:go_highlight_build_constraints = 1
"let g:go_fmt_command = "goimports"
"let g:go_fmt_fail_silently = 1
"" Setting Vim-go End-------------------------------------------

" Setting AirLine Begin----------------------------------------
set laststatus=2
let g:airline_theme='light'
let g:airline#extensions#tabline#enabled = 1
let g:airline_powerline_fonts = 1
let g:airline#extensions#tabline#left_sep = ' '
let g:airline#extensions#tabline#left_alt_sep = '|'
let g:airline#extensions#tabline#buffer_nr_show = 1
" Setting AirLine End------------------------------------------

" Setting CTRP Begin-------------------------------------------
let g:ctrlp_map = '<c-p>'
let g:ctrlp_cmd = 'CtrlP'
map <leader>f :CtrlPMRU<CR>
let g:ctrlp_custom_ignore = {
    \ 'dir':  '\v[\/]\.(git|hg|svn|rvm)$',
    \ 'file': '\v\.(exe|so|dll|zip|tar|tar.gz|pyc)$',
    \ }
let g:ctrlp_working_path_mode = 'ra'
let g:ctrlp_match_window_bottom=1
let g:ctrlp_max_height=15
let g:ctrlp_match_window_reversed=0
let g:ctrlp_mruf_max=500
let g:ctrlp_follow_symlinks=1
let g:ctrlp_max_depth = 40
" Setting CTRP End---------------------------------------------

" Setting taglist End------------------------------------------
let Tlist_Use_Right_Window = 1
let Tlist_File_Fold_Auto_Close = 1 
let Tlist_Show_One_File = 1
let Tlist_Exit_OnlyWindow = 1
let Tlist_WinWidth = 25
:map <F3> :Tlist<cr> 

""Setting ycm Begin---------------------------------------------
""let g:ycm_auto_trigger = 0
""let g:ycm_global_ycm_extra_conf = "~/.vim/bundle/YouCompleteMe/third_party/ycmd/cpp/ycm/.ycm_extra_conf.py"
""let g:ycm_autoclose_preview_window_after_insertion = 0
""let g:ycm_key_list_select_completion = ['<C-n>', '<C-j>']
""let g:ycm_key_list_previous_completion = ['<C-p>', '<C-k>']
""let g:ycm_key_invoke_completion = '<F9>'
""let g:ycm_keep_logfiles = 1
""set completeopt=longest,menu
""inoremap <expr> <CR> pumvisible() ? "\<C-y>" : "\<CR>"
""nnoremap <leader>gl :YcmCompleter GoToDeclaration<CR>
""nnoremap <leader>gf :YcmCompleter GoToDefinition<CR>
""nnoremap <leader>gg :YcmCompleter GoToDefinitionElseDeclaration<CR>
""let g:ycm_filetype_blacklist = {
""			\ 'tagbar' : 1,
""			\ 'qf' : 1,
""			\ 'notes' : 1,
""			\ 'markdown' : 1,
""			\ 'unite' : 1,
""			\ 'text' : 1,
""			\ 'vimwiki' : 1,
""			\ 'pandoc' : 1,
""			\ 'infolog' : 1,
""			\ 'mail' : 1,
""			\ 'mundo': 1,
""			\ 'fzf': 1,
""			\ 'ctrlp' : 1
""			\}
