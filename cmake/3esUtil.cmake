# A wrapper for set() that does nothing if VAR already has a value.
macro(ensure_set VAR)
  if(NOT DEFINED ${VAR})
    set(${VAR} ${ARGN})
  endif(NOT DEFINED ${VAR})
endmacro(ensure_set)
