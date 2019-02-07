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
  #:use-module (geda object)
  #:use-module (gschem core gettext)
  #:use-module (gschem action)
  #:use-module (gschem gschemdoc)
  #:use-module (gschem repl)
  #:use-module (gschem selection)
  #:use-module (gschem window)
  #:use-module (srfi srfi-1))

(or (defined? 'define-syntax)
    (use-modules (ice-9 syncase)))

(define-syntax define-action-public
  (syntax-rules ()
    ((_ (name . args) . forms)
     (begin
       (define-action (name . args) . forms)
       (export name)))))

(define-action-public (&file-repl #:label (_ "Terminal REPL") #:icon "gtk-execute")
  (start-repl-in-background-terminal))

(define-action-public
    (&hierarchy-documentation #:label (_ "Component Documentation")
                              #:icon "symbol-datasheet"
                              #:tooltip (_ "View documentation for selected component"))

  "If a component is selected, search for and display corresponding
documentation in a browser or PDF viewer. If no documentation can be
found, shows a dialog with an error message."

  (catch 'misc-error

   (lambda ()
     (let ((component
            (any (lambda (obj) (and (component? obj) obj))
                 (page-selection (active-page)))))
       (and component (show-component-documentation component))))

   (lambda (key subr msg args . rest)
     (gschem-msg (string-append
                  (_ "Could not show documentation for selected component:\n\n")
                  (apply format #f msg args))))))

(define-action-public
    (&help-manual #:label (_ "gEDA Manuals") #:icon "help-browser"
     #:tooltip (_ "View the front page of the gEDA documentation in a browser."))
  (show-wiki "geda:documentation"))

(define-action-public
    (&help-guide #:label (_ "gschem User Guide") #:icon "gtk-help"
                 #:tooltip (_ "View the gschem User Guide in a browser."))
  (show-wiki "geda:gschem_ug"))

(define-action-public
    (&help-faq #:label (_ "gschem FAQ") #:icon "help-faq"
     #:tooltip (_ "Frequently Asked Questions about using gschem."))
  (show-wiki "geda:faq-gschem"))

(define-action-public
    (&help-wiki #:label (_ "gEDA wiki") #:icon "web-browser"
     #:tooltip (_ "View the front page of the gEDA wiki in a browser."))
  (show-wiki))

;; Local Variables:
;; eval: (put 'define-action-public 'scheme-indent-function 1)
;; End:
