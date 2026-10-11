set expandtab

let &path.= "godot-cpp/gen/include,godot-cpp/gdextension/godot-cpp/inlcude,"
let g:ale_c_build_dir_names = ['build-clang-tidy', 'build', 'bin']

let g:ale_fixers = {
    \ 'hpp': ['clang-format'],
    \ 'h': ['clang-format'],
    \ 'c': ['clang-format'],
    \ 'cpp': ['clang-format']
    \}

let g:ale_linters = {
    \ 'h': ['clangtidy'],
    \ 'hpp': ['clangtidy'],
    \ 'c': ['clangtidy'],
    \ 'cpp': ['clangtidy']
    \}

let g:ale_c_clangformat_options = '--style=file'
let g:ale_cpp_clangformat_options = '--style=file'
let g:ale_c_clangtidy_extra_options = '-extra-arg=-Wno-macro-redefined'
let g:ale_cpp_clangtidy_extra_options = '-extra-arg=-Wno-macro-redefined'

set colorcolumn=120
set textwidth=120

autocmd FileType xml setlocal shiftwidth=4 noexpandtab

let g:ale_fix_on_save = 1

function! s:configure_format_on_save() abort
  let b:ale_fix_on_save = expand('%:p') =~# '/src/legacy/maszyna-mover/' ? 0 : 1
endfunction

augroup AleFixOnSaveMaszyna
  autocmd!
  autocmd BufRead,BufNewFile *.c,*.h,*.cpp,*.hpp call s:configure_format_on_save()
augroup END

let g:maszyna_godot_lsp_port = get(g:, 'maszyna_godot_lsp_port', 6005)
let g:maszyna_godot_project_path = get(g:, 'maszyna_godot_project_path', expand('<sfile>:p:h') . '/demo')

function! s:godot_lsp_address(buffer) abort
  return '127.0.0.1:' . g:maszyna_godot_lsp_port
endfunction

function! s:godot_project_root(buffer) abort
  return g:maszyna_godot_project_path
endfunction

function! s:register_godot_ale_lsp() abort
  if !exists('g:loaded_ale')
    return
  endif

  if !exists('g:maszyna_godot_ale_registered')
    call ale#linter#Define('gdscript', {
          \ 'name': 'godot',
          \ 'lsp': 'socket',
          \ 'address': function('s:godot_lsp_address'),
          \ 'project_root': function('s:godot_project_root'),
          \ })
    call ale#linter#Define('gsl', {
          \ 'name': 'godot',
          \ 'lsp': 'socket',
          \ 'address': function('s:godot_lsp_address'),
          \ 'project_root': function('s:godot_project_root'),
          \ })
    let g:maszyna_godot_ale_registered = 1
  endif
endfunction

function! s:configure_godot_ale_lsp() abort
  call s:register_godot_ale_lsp()
  let b:ale_linters = {'gdscript': ['godot'], 'gsl': ['godot']}
  let b:ale_completion_autoimport = 1
  setlocal omnifunc=ale#completion#OmniFunc
  nnoremap <buffer> <silent> gd <Plug>(ale_go_to_definition)
  nnoremap <buffer> <silent> gr <Plug>(ale_find_references)
  nnoremap <buffer> <silent> K <Plug>(ale_hover)
  nnoremap <buffer> <silent> <F2> <Plug>(ale_rename)
endfunction

augroup MaszynaGodotAleLsp
  autocmd!
  autocmd FileType gdscript,gsl call s:configure_godot_ale_lsp()
augroup END
