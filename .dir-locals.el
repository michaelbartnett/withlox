((nil . ((projectile-project-compilation-cmd
          . "./scripts/build_makefiles.sh")
         (projectile-project-run-cmd
          . "gdb --batch -ex \"run $(cat ./run_args.txt )\" build/makefiles/jsoneditor")
          ;; . "./scripts/run_makefiles.sh")
         ))
 )
