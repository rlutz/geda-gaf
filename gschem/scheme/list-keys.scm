;; Printing out current key bindings for gEDA (gschem)
; Stefan Petersen 1999-04-04 (spe@stacken.kth.se)
; Free for all use. Just don't blame me when your house burns up.
; Updated by Roland Lutz 2019-02-13

(define (print-mapped-keys binding mapped-keys)
  (display binding)
  (display " = ")
  (for-each (lambda (key)
	      (display key)
	      (display " "))
	    mapped-keys)
  (newline))

(define (mapping-keys keymap keys)
  (keymap-for-each
   (lambda (key binding)
     (cond ((keymap? binding)
	    (mapping-keys binding (append keys (list key))))
	   (else
	    (print-mapped-keys binding (append keys (list key))))))
   keymap))

(mapping-keys %global-keymap '())
