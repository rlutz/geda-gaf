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

(define-module (gschem builtins)
  #:use-module (gschem core gettext)
  #:use-module (gschem action)
  #:use-module (gschem gschemdoc))

(or (defined? 'define-syntax)
    (use-modules (ice-9 syncase)))

(define-syntax define-action-public
  (syntax-rules ()
    ((_ (name . args) . forms)
     (begin
       (define-action (name . args) . forms)
       (export name)))))

(define-action-public
    (&help-manual
     #:icon       "help-browser"
     #:name       (_ "gEDA Manuals")
     #:label      (_ "gEDA Documentation...")
     #:menu-label (_ "gEDA Docu_mentation...")
     #:tooltip    (_ "View the front page of the gEDA documentation in a browser"))
  (show-wiki "geda:documentation"))

(define-action-public
    (&help-guide
     #:icon       "gtk-help"
     #:name       (_ "gschem User Guide")
     #:label      (_ "gschem User Guide...")
     #:menu-label (_ "gschem User _Guide...")
     #:tooltip    (_ "View the gschem User Guide in a browser"))
  (show-wiki "geda:gschem_ug"))

(define-action-public
    (&help-faq
     #:icon       "help-faq"
     #:name       (_ "gschem FAQ")
     #:label      (_ "gschem FAQ...")
     #:menu-label (_ "gschem _FAQ...")
     #:tooltip    (_ "Frequently Asked Questions about using gschem"))
  (show-wiki "geda:faq-gschem"))

(define-action-public
    (&help-wiki
     #:icon       "web-browser"
     #:name       (_ "gEDA Wiki")
     #:label      (_ "gEDA Wiki...")
     #:menu-label (_ "gEDA _Wiki...")
     #:tooltip    (_ "View the front page of the gEDA wiki in a browser"))
  (show-wiki))

;; Local Variables:
;; eval: (put 'define-action-public 'scheme-indent-function 1)
;; End:
