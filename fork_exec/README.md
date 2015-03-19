How to write for plantuml
---

PlantUMLis a component that allows to quickly write :

* Sequence diagram,
* Usecase diagram,
* Class diagram,
* Activity diagram, (here is the new syntax),
* Component diagram,
* State diagram,
* Object diagram.
* Wireframe graphical interfac

See the [official website] (http://www.plantuml.com) for more details.

How to use plantuml
---

The *diagram* rule in the Makefile assumed that the *plantuml* executable is
in your `$PATH`.  This file could contain something like:

    #!/bin/sh
    java -jar $HOME/lib/java/plantuml.jar $@

Don't forget to add the executable flag with this command:

    chmod +x /path/to/plantuml

Download
---
* See [here](http://www.plantuml.com/download.html) for Plantuml download 
* See [here](http://www.vim.org/scripts/script.php?script_id=3538) for Vim users
* See [here](http://plantuml.sourceforge.net/emacs.html) for Emacs users 

