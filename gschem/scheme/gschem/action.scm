;; gEDA - GPL Electronic Design Automation
;; gschem - gEDA Schematic Capture - Scheme API
;; Copyright (C) 2013 Peter Brett <peter@peter-b.co.uk>
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
;;

(define-module (gschem action)
  #:use-module (gschem core gettext)
  #:use-module (gschem core action)
  #:use-module (gschem window)
  #:use-module (ice-9 optargs)
  #:export-syntax (define-action))

(or (defined? 'define-syntax)
    (use-modules (ice-9 syncase)))

(define current-action-position (make-fluid))

;; Evaluates a gschem action.  A gschem action is expected to be an
;; action object from (gschem core builtins) or returned by make-action.
(define-public (eval-action! action)
  (if (action? action)
      (begin
        (action) ;; Actually execute the action
        #t)
      (error (_ "~S is not a valid gschem action.") action)))

;; Evaluate an action at a particular point on the schematic plane.
;; If the point is omitted, the action is evaluated at the current
;; mouse pointer position.
(define*-public (eval-action-at-point!
                 action
                 #:optional (point (pointer-position)))

  (with-fluids ((current-action-position point))
               (eval-action! action)))

;; Return the current action pointer position.  This should be the
;; location at which the action was invoked (set via
;; eval-action-at-point!).
(define-public (action-position)
  (fluid-ref current-action-position))

;; -------------------------------------------------------------------
;; First-class actions

(define-public action? %action?)

(define-syntax define-action
  (syntax-rules ()
    ((_ (name . args) . forms)
     (define name (make-action (lambda () . forms) . args)))))

(define-public (make-action thunk . props)
  (let rec ((props props)
            (alist '()))
    (if (>= (length props) 2)
        (rec (cddr props) (assq-set! alist (car props) (cadr props)))
        (%make-action (assq-ref alist '#:icon)
                      (assq-ref alist '#:name)
                      (assq-ref alist '#:label)
                      (assq-ref alist '#:menu-label)
                      (assq-ref alist '#:tooltip) thunk))))
