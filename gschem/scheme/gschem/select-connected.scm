
; Copyright G P Eckersley
; Released under terms of GNU public license version 3 and later
; No warranties or liabilty of any sort

; References used to write this:

; https://en.wikibooks.org/wiki/Scheme_Programming/List_Operations
; https://ljh4timm.home.xs4all.nl/gaf/geda-scheme-api-1.8.1/index.html
; https://www.gnu.org/software/guile/manual/html_node/List-Selection.html
; http://wiki.geda-project.org/geda:na_howto
; http://www-mdp.eng.cam.ac.uk/web/CD/engapps/geda/geda-doc/keymapping/keymapping.html
; http://wiki.geda-project.org/geda:gschem_ug:config
; https://www.gnu.org/software/guile/manual/html_node/Load-Paths.html

; /etc/gEDA/system-gschemrc


(define-module (gschem select-connected))

(export selcncts)

(use-modules (geda page))
(use-modules (geda object))
(use-modules (geda attrib))
(use-modules (gschem selection))
(use-modules (gschem window))




(define (td dta)
  (if (equal? #t #t)
    (begin (display dta) (display " ")) ))

(define void #f) ; an empty list which can be appended to by thereto without,
; without changing the reference unlike the lisp '()
; This is only used where a reference must be maintained
(define (empty) (list void))  ; empty list initialiser
(define (void? objl)         ; function to test if empty list
  (or (null? objl) (equal? objl (list void)))
)
(define (objdict) (list (empty) (empty))) ; keys,object lists

(define (thereto lst appendix) (let ((prev 1)) ; append single object to list
  (define (thereto1 lst appendix)
     (if (void? lst)
         (set-cdr! prev (list appendix))
         (begin
           (set! prev lst)
           (thereto1 (cdr lst) appendix)
         )))
  (if (void? lst )
    (list-set! lst 0 appendix)
    (thereto1 lst appendix)
  )
  (length lst)
))

(define  (listinsert obj objl)  ; See if in list and return index if found else #f
  (let ((found #f) (res #f) (ctr 0))
    (define (lp1 lst)
      (if (or found (null? lst))
         (begin
           (if found (set! res (- ctr 1)))
         )
         (begin
           (set! found (equal? obj (car lst)))
           (set! ctr (+ ctr 1))
           (lp1 (cdr lst))
         )))
    (if (not (void? objl))
      (begin (lp1 objl) res)
    )
    res
  ))

(define (listput obj objl) ; put obj into list objl if not already there.
  (let ((res 0))
    (set! res  (listinsert obj objl))
    (if (equal? res #f)
      (thereto objl obj)
    )
    res
  ))

(define (dictput dict key item) ; put object into dictionary
  (let ((a1 0) (a2 0) (itmlst 0) (keys (list-ref  dict 0)) (itmlsts (list-ref dict 1)))
    (set! a1  (listput key keys))
    (if a1 (begin
      (listput item  (list-ref itmlsts a1))
    )
    (begin
      (thereto itmlsts  (list item))
    ))))

(define (gcnets rdict comp) (let ((names 0) (items (component-contents comp))
; get nets defined by component. e/g/ vcc/gnd etc
       (vals 0) (a3 0) (net-names (empty)) (net-pins (empty)))
   (define (pinget items) (let ((a1 0) (a2 0) (a3 0) (pind 0) (netind 0);  get pins on component
            (names 0) (vals 0)  (val 0) (netname 0) )
       (if (not (null? items)) (begin
         (set! a1 (car items))
         (if (eq? (object-type a1) (quote pin)) (begin
           (set! a2 (object-attribs a1))
           (set! names (map attrib-name a2))
           (set! vals (map attrib-value a2))
           (set! pind (listinsert "pinnumber" names))
           (set! val (list-ref vals pind))
           (set! a3 (listinsert val net-pins))
           (if (not (equal? a3 #f)) (begin
             (set! netname  (list-ref net-names a3))
             (dictput rdict netname a1)
             ))))
       (pinget (cdr items))
       ))))
   (define (nget names) (let ((a1 0) (a2 0) (nnme 0)) ; search attribute names for pins
       (if (not (null? names)) (begin
         (set! a1 (car names))
         (if (equal? a1 "net") (begin
         (set! nnme (string-split (list-ref vals a2) #\:))
         (thereto net-names (list-ref nnme 0))
         (thereto net-pins (list-ref nnme 1))
         ))
       (set! a2 (+ a2 1))
       (nget (cdr names))
       ))))

   (set! a3 (inherited-attribs comp))
   (set! names (map attrib-name a3))
   (set! vals (map attrib-value a3))
   (nget names)   ; get nets on component
   (pinget items)
))

(define (glnets rdict track) ; get any netted tracks
  (let ((a1 0)  (c1 0) (attribs (object-attribs track))
     (names 0) (vals 0) (val 0))
     (define (gnets names)
        (if (not (null? names))
            (begin
              (set! a1 (car names))
              (if (equal? a1 "netname") (begin
                (set! val (list-ref vals c1))
                (dictput rdict val track)
                ))
              (set! c1 (+ c1 1))
              (gnets (cdr names))
            )))
     (set! names (map attrib-name attribs))
     (set! vals (map attrib-value attribs))
     (gnets names)
  ))

(define (loadnets)
  (let ((a1 0) (a2 0) (aps (active-pages))
   (rdict (objdict)) (allsel (list)) (visible (list)) (background (list)))
     (define (pges aps)
       (if (not (null? aps))
         (begin  ; get selecte objects in bages
          (set! allsel (append allsel (page-contents (car aps))) )
          (if (equal? (car aps) (active-page))
             (set! visible (append visible (page-selection (car aps))))
             (set! background (append background (page-selection (car aps))))
          )
          (pges (cdr aps))
         )))
     (define (objt objl)  (let ((a1 0) (gnp 0) ) ; get nets associated to object
       (if (not (void? objl))
         (begin
          (set! a1 (car objl))
          (set! a2 (object-type a1))
          (if (eq? a2 (quote net))
             (begin
                (glnets rdict a1)        ; nets on tracks
             ))
          (if (eq? a2 (quote complex))   ; nets on compnent pins
             (begin
                (gcnets rdict a1)
             ))
          (objt (cdr objl))
         ))))
     (pges aps)
     (objt allsel)
  (list rdict allsel visible background)
  ))

(define (selectn objl) ; select all selectables in list
   (define (sel1 objl) (let ((tobj 0) (p2 0))
     (if (not (null? objl))
       (begin (set! tobj (car objl))
         (set! p2 (object-type tobj))
         (if (eq? p2 (quote net))   ; net, pin , line etc
          (begin  (select-object! tobj )))
         (sel1 (cdr objl))
       ))))
   (sel1 objl))


(define (deselectn objl) ; deselect all selectables in list
   (define (desel1 objl) (let ((tobj 0) (p2 0))
     (if (not (null? objl))
       (begin (set! tobj (car objl))
         (set! p2 (object-type tobj))
         (deselect-object! tobj )
         (desel1 (cdr objl))
       ))))
   (desel1 objl))

(define (selcncts) (let ((a1 0)
; select all objects connected to current selection in visible page
; after deselecting all selections in background page
    (nsel (loadnets)) ; [nets ; originally selected objects]
    (blist (empty))    ; all checked
    (netlist '())  ; netnames found
  )

  (define (net-conns obj) (let ((x1 0) (x2 0) (res '()) )
     (define (srchn nl) (let ((w1 0) (w2 0) )
       (if (not (null? nl)) (begin
           (set! w1 (listinsert obj (car nl)))
           (if (not (equal? w1 #f))
             (set! res (append res (car nl))))
          (srchn (cdr nl))
       ))
       res
       ))
     (set! x1 (object-connections obj))
     (if (not (void? netlist)) (begin
       (set! x2 (srchn (car (cdr netlist))))
       (set! x1 (append x2 x1))
       ))
     x1
  ))
  (define (chkobjs bjsel) (let ((b1 0) (b2 0) (b3 0) (b4 0))
    (if (not (null? bjsel)) (begin
      (set! b1 (car bjsel))
      (set! b2 (listput b1 blist))
      (if (not b2) (begin
        (set! b3 (net-conns b1))
        (chkobjs b3)
        (selectn b3)
        ))
      (chkobjs (cdr bjsel))
      ))))
  (set! a1 (list-ref nsel 2))  ; ausw,vis,bg
  (deselectn (list-ref nsel 3))
  (set! netlist (car  nsel))      ; net connected objects groups
  (chkobjs a1)
))
