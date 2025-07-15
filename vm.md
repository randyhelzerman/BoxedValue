Stack based architceture?

push
pop


add
sub
mul
div

op1  numb
op2  numb
dst  numb

typ  numb type



##   Split-Stream Orientation

###   when we split, we don't unshare the streams; indeed, we
      will sync up on reading the streams.   

stream in reg address
stream out reg address

match    char stream
split    address
jump     address
succeed
fail