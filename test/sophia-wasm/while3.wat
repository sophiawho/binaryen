(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "main" (func $main))
 (func $main (; 0 ;) (result i32)
  (local $0 i32)
  (local $1 i32)
  (set_local $1
   (i32.const 3)
  )
  (set_local $0
   (i32.const 1)
  )
  (loop $label$0 (result i32)
   (set_local $0
    (i32.add
     (get_local $0)
     (i32.const 1)
    )
   )
   (loop $label$1
    (br_if $label$1
     (i32.ge_s
      (tee_local $0
       (i32.add
        (get_local $0)
        (i32.const -1)
       )
      )
      (get_local $1)
     )
    )
   )
   (loop $label$2 (result i32)
    (br_if $label$2
     (i32.lt_s
      (tee_local $0
       (i32.add
        (get_local $0)
        (i32.const 1)
       )
      )
      (tee_local $1
       (i32.add
        (get_local $1)
        (i32.const -1)
       )
      )
     )
    )
    (br $label$0)
   )
  )
 )
)

