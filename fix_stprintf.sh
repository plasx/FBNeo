#!/bin/bash
sed -i.bak "s/#define _stprintf sprintf/#define _stprintf snprintf/" src/burner/gami.mm
