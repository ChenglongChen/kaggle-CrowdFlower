
  use strict 'vars'; 

  my $dlm = ','; 
  my $inp_ext = '.inp'; 

  my $arg_num = $#ARGV + 1; 
  if ($arg_num != 3) {
    print "Arguments: exe train|predict|train_test cfg_fn\n"; 
    print "   exe   : Name of the executable.  Typically, ../bin/rgf \n"; 
    print "   train|predict|train_test \n"; 
    print "           train      ... Train and save models to files.  \n"; 
    print "           predict    ... Apply a model to new data. \n"; 
    print "           train_test ... Train and test models in one call. \n"; 
    print "   cfg_fn: Path to a configuration file without extension; e.g., sample/train \n"; 
    print "           The file extension should be \".inp\"\n";   
    print "           For example, see sample/train.inp and sample/predict.inp.\n"; 
    print "\n"; 
    print "           To get help on the parameters that can be used in configuration\n"; 
    print "           files, call the executable with train|predict|train_test, e.g.,\n"; 
    print "\n"; 
    print "                           ..\\bin\\rgf  train \n"; 
    print "                           ../bin/rgf  predict \n"; 
    exit 1; 
  }

  my $argx = 0; 
  my $exe = $ARGV[$argx++]; 
  my $action = $ARGV[$argx++]; 
  my $inp_fn = $ARGV[$argx++]; 

  $inp_fn .= $inp_ext; 

  my @list = &readList_inc($inp_fn); 
  my $num = $#list + 1; 

  my $config = ""; 

  my $repeat_num = 0; 
  my(@repeat); 
  my $ix; 
  for ($ix = 0; $ix < $num; ++$ix) {
    my $kw = ""; 
    if ($list[$ix] =~ /^\@(.*)$/) {
      $repeat[$repeat_num++] = $1; 
    }
    else {
      &concatKwVal($list[$ix]); 
    }
  }


  if ($repeat_num == 0) {
    my $cmd = "$exe $action $config"; 
    print "$cmd\n"; 
    system($cmd); 
  }
  else {
    my $rx; 
    for ($rx = 0; $rx < $repeat_num; ++$rx) {
      my $my_config = $repeat[$rx] . ',' . $config; 

      my $cmd = "$exe $action $my_config"; 
      print "$cmd\n"; 
      system($cmd); 
      my $ret = $?; 
      if ($ret != 0) {
        print "system() failed; exit call_exe.pl\n"; 
        exit 1; 
      }
    }
  }

  exit 0; 

##-----------------------------------
##  if there is a line "#include filename" ... 
sub readList_inc {
  my($fn) = @_; 
  my(@out); 

  my @list = &readList($fn); 

  my $num = $#list + 1; 
  my $out_ix = 0; 
  my $ix; 
  for ($ix = 0; $ix < $num; ++$ix) {
    if ($list[$ix] =~ /^\include\s+(\S+)$/) {
      my $inc_fn = $1; 
      my(@list1); 
      @list1 = &readList($inc_fn); 
      my $num1 = $#list1 + 1; 
      my $jx; 
      for ($jx = 0; $jx < $num1; ++$jx) {
        if ($list1[$jx] =~ /^include/) {
          print "Can't nest include: $inc_fn\n"; 
          exit 1; 
        }
        $out[$out_ix++] = &checkInputLine($list1[$jx]); 
      }
    }
    else {
      $out[$out_ix++] = &checkInputLine($list[$ix]); 
    }
  }
  return @out; 
}

##------
sub concatKwVal {
  my($inp) = @_; 
  if ($inp =~ /\S/) {
    if ($config ne "") {
      $config .= $dlm; 
    }
    $config .= $inp; 
  }
}

#####
sub readList {
  my($lst_fn) = @_; 
  my(@item);   

  if (!open(LST, "$lst_fn")) {
    print "Can't open $lst_fn\n"; 
    exit 1; 
  }

  my $num = 0; 
  while(<LST>) {
    my $line = $_; 
    chomp $line; 
    if ($line !~ /^\s*\#/) {
      $line = &strip($line); 
      $item[$num++] = $line; 
    }
  }

  close(LST); 
  return @item; 
}

#####
sub strip {
  my($inp) = @_; 
  my $out = $inp; 
  if ($inp =~ /^\s*(\S.*\S)\s*$/) {
    $out = $1; 
  }
  elsif ($inp =~ /^\s*(\S)\s*$/) {
    $out = $1; 
  }
  return $out; 
}

#####
sub checkInputLine {
  my($inp) = @_; 

  my $param = $inp; 
  if ($param =~ /^(\S+)\s+(\S+)/) {
    $param = $1; 
    my $comment = $2; 
    if ($comment !~ /^\#/) {
      print "No space is allowed in the parameter.  Comments should start with \#\n"; 
      print "Error in: [$inp]\n"; 
      exit; 
    }
  }
  if ($param =~ /^([^\#]+)\#/) {
    $param = $1; 
  }
  return $param; 
}
