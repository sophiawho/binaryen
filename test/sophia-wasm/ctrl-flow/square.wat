(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "_Z6squarei" (func $_Z6squarei))
 (export "main" (func $main))
 (func $_Z6squarei (; 0 ;) (param $0 i32) (result i32)
  (i32.mul
   (get_local $0)
   (get_local $0)
  )
 )
 (func $main (; 1 ;) (result i32)
  (i32.const 9)
 )
)

