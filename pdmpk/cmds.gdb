# gdb Use this file either as a command line argument `-x $0` or with
# `source $file` inside gdb

undisplay
delete breakpoints
start m5p4.loop.mtx 4 4
break partial_cd::partial_cd
continue
break partial_cd.cc:134 if tgt_part==20
display bufs[1].mpi_bufs.init_idcs
