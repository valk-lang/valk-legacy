
#if OS == win
link_static "libuv"
#else
link_static "uv"
#end
