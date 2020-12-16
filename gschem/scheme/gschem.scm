;;; gEDA - GPL Electronic Design Automation
;;; gschem - gEDA Schematic Capture
;;; Copyright (C) 1998-2010 Ales Hvezda
;;; Copyright (C) 1998-2020 gEDA Contributors (see ChangeLog for details)
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
;;; MA 02111-1301 USA.

(use-modules (gschem keymap)
             (gschem action)
             (gschem core gettext)
             (gschem core builtins)
             (gschem builtins)
             (gschem window)
             (srfi srfi-1))

;; -------------------------------------------------------------------
;;;; Global keymaps and key dispatch logic

(define current-keys '())

(define %global-keymap (make-keymap))
(define current-keymap %global-keymap)

;; Set a global keybinding
(define (global-set-key key binding)
  (bind-keys! %global-keymap key binding))

;; Called from C code to evaluate keys.
(define (press-key key)
  (eval-pressed-key current-keymap key))

;; Function for resetting current key sequence
(define (reset-keys) (set! current-keys '()) #f)

;; Does the work of evaluating a key.  Adds the key to the current key
;; sequence, then looks up the key sequence in the current keymap.  If
;; the key sequence resolves to an action, calls the action.  If the
;; key sequence can be resolved to an action, returns #t; if it
;; resolves to a keymap (i.e. it's a prefix key), returns the "prefix"
;; symbol; otherwise, returns #f.  If the key is #f, clears the
;; current key sequence.
(define (eval-pressed-key keymap key)
  (if key
      (begin
        ;; Add key to current key sequence
        (set! current-keys (cons key current-keys))
        (let* ((keys (list->vector (reverse current-keys)))
               (bound (lookup-keys keymap keys)))
          (cond
           ;; Keys are a prefix -- do nothing successfully
           ((keymap? bound) 'prefix)
           ;; Keys are bound to something -- reset current key
           ;; sequence, then try to run the action
           (bound (begin
                    (reset-keys)
                    (eval-action-at-point! bound)))
           ;; No binding
           (else (reset-keys)))))

      (reset-keys)))


(define (eval-stroke stroke)
  (let ((action (assoc stroke strokes)))
    (cond ((not action)
;           (display "No such stroke\n")
;          (display stroke)
           #f)
          (else
;           (display "Scheme found action ")
;           (display action)
;           (display "\n")
           (eval-action! (cdr action))
           #t))))

;; Search the global keymap for a particular symbol and return the
;; keys which execute this hotkey, as a string suitable for display to
;; the user. This is used by the gschem menu system.
(define (find-key action)
  (let ((keys (lookup-binding %global-keymap action)))
    (and keys (keys->display-string keys))))

;; Printing out current key bindings for gEDA (gschem)
(define (%gschem-hotkey-store/dump-global-keymap)
  (dump-keymap %global-keymap))

(define (dump-keymap keymap)

  ;; Use this to change "Page_Up" to "Page Up" (etc.)
  (define (munge-keystring str)
    (string-map (lambda (c) (case c ((#\_) #\ ) (else c))) str))

  (define lst '())

  (define (binding->entry prefix key binding)
    (let* ((keys (list->vector (reverse (cons key prefix))))
           (keystr (munge-keystring (keys->display-string keys))))
      (set! lst (cons (list binding keystr) lst))))

  (define (build-dump! km prefix)
    (keymap-for-each
     (lambda (key binding)
       (cond

        ((action? binding)
         (binding->entry prefix key binding))

        ((keymap? binding)
         (build-dump! binding (cons key prefix)))
        (else (error "Invalid action ~S bound to ~S"
                     binding (list->vector (reverse (cons key prefix)))))))
     km))

  (build-dump! keymap '())
  lst)


;; Define old keymapping callbacks.
;;
;; These are for compatibility only--don't use them, don't change the
;; Scheme names.

(define file-new-window &file-new-window)
(define file-new &file-new)
(define file-open &file-open)
(define file-script &file-script)
(define file-save &file-save)
(define file-save-as &file-save-as)
(define file-save-all &file-save-all)
(define file-print &file-print)
(define file-image &file-image)
(define file-close-window &file-close-window)
(define file-quit &file-quit)
(define edit-undo &edit-undo)
(define edit-redo &edit-redo)
(define edit-select &edit-select)
(define edit-select-all &edit-select-all)
(define edit-deselect &edit-deselect)
(define edit-copy &edit-copy)
(define edit-mcopy &edit-mcopy)
(define edit-move &edit-move)
(define edit-delete &edit-delete)
(define edit-rotate-90 &edit-rotate-90)
(define edit-mirror &edit-mirror)
(define edit-slot &edit-slot)
(define edit-color &edit-properties)
(define edit-edit &edit-edit)
(define edit-text &edit-text)
(define edit-lock &edit-lock)
(define edit-unlock &edit-unlock)
(define edit-linetype &edit-properties)
(define edit-filltype &edit-properties)
(define edit-pin-type &edit-properties)
(define edit-translate &edit-translate)
(define edit-invoke-macro &edit-invoke-macro)
(define edit-embed &edit-embed)
(define edit-unembed &edit-unembed)
(define edit-update &edit-update)
(define edit-show-hidden &edit-show-hidden)
(define edit-find-text &edit-find-text)
(define edit-show-text &edit-show-text)
(define edit-hide-text &edit-hide-text)
(define edit-autonumber &edit-autonumber)

(define clipboard-copy &clipboard-copy)
(define clipboard-cut &clipboard-cut)
(define clipboard-paste &clipboard-paste)

(define buffer-copy1 &buffer-copy1)
(define buffer-copy2 &buffer-copy2)
(define buffer-copy3 &buffer-copy3)
(define buffer-copy4 &buffer-copy4)
(define buffer-copy5 &buffer-copy5)
(define buffer-cut1 &buffer-cut1)
(define buffer-cut2 &buffer-cut2)
(define buffer-cut3 &buffer-cut3)
(define buffer-cut4 &buffer-cut4)
(define buffer-cut5 &buffer-cut5)
(define buffer-paste1 &buffer-paste1)
(define buffer-paste2 &buffer-paste2)
(define buffer-paste3 &buffer-paste3)
(define buffer-paste4 &buffer-paste4)
(define buffer-paste5 &buffer-paste5)

(define view-redraw &view-redraw)
(define view-zoom-full &view-zoom-full)
(define view-zoom-extents &view-zoom-extents)
(define view-zoom-in &view-zoom-in)
(define view-zoom-out &view-zoom-out)
(define view-zoom-box &view-zoom-box)
(define view-pan &view-pan)
(define view-pan-left &view-pan-left)
(define view-pan-right &view-pan-right)
(define view-pan-up &view-pan-up)
(define view-pan-down &view-pan-down)
(define view-dark-colors &view-dark-colors)
(define view-light-colors &view-light-colors)
(define view-bw-colors &view-light-bw-colors)
(define page-manager &page-manager)
(define page-next &page-next)
(define page-prev &page-prev)
(define page-close &page-close)
(define page-revert &page-revert)
(define page-print &page-print)
(define add-component &add-component)
(define add-attribute &add-attribute)
(define add-net &add-net)
(define add-bus &add-bus)
(define add-text &add-text)
(define add-path &add-path)
(define add-line &add-line)
(define add-box &add-box)
(define add-picture &add-picture)
(define add-circle &add-circle)
(define add-arc &add-arc)
(define add-pin &add-pin)
(define hierarchy-down-schematic &hierarchy-down-schematic)
(define hierarchy-down-symbol &hierarchy-down-symbol)
(define hierarchy-up &hierarchy-up)
(define attributes-attach &attributes-attach)
(define attributes-detach &attributes-detach)
(define attributes-show-name &attributes-show-name)
(define attributes-show-value &attributes-show-value)
(define attributes-show-both &attributes-show-both)
(define attributes-visibility-toggle &attributes-visibility-toggle)
(define options-snap-size &options-options)
(define options-scale-up-snap-size &options-scale-up-snap-size)
(define options-scale-down-snap-size &options-scale-down-snap-size)
(define options-action-feedback &options-action-feedback)
(define options-grid &options-grid)
(define options-snap &options-snap)
(define options-rubberband &options-rubberband)
(define options-magneticnet &options-magneticnet)
(define options-show-log-window &options-show-log-window)
(define options-show-coord-window &options-show-coordinates)
(define help-about &help-about)
(define help-hotkeys &help-hotkeys)
(define cancel &cancel)
