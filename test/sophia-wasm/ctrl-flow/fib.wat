
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "_Z3fibi" (func $_Z3fibi))
 (func $_Z3fibi (; 0 ;) (param $0 i32) (result i64)
  (local $1 i64)
  (set_local $1
   (i64.const 1)
  )
  (block $label$0
   (br_if $label$0
    (i32.lt_s
     (get_local $0)
     (i32.const 2)
    )
   )
   (set_local $0
    (i32.add
     (get_local $0)
     (i32.const 2)
    )
   )
   (set_local $1
    (i64.const 1)
   )
   (loop $label$1
    (set_local $1
     (i64.add
      (call $_Z3fibi
       (i32.add
        (get_local $0)
        (i32.const -3)
       )
      )
      (get_local $1)
     )
    )
    (br_if $label$1
     (i32.gt_s
      (tee_local $0
       (i32.add
        (get_local $0)
        (i32.const -2)
       )
      )
      (i32.const 3)
     )
    )
   )
  )
  (get_local $1)
 )
)
