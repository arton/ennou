skip = Proc.new do |line|
  skip
end
inp = Proc.new do |line|
  if /^}/ =~ line
    skip
  else
    if /^\s+HttpVerb([^\s,]+)/ =~ line
      puts(%!    "#{$1.upcase}",!)
    end  
    inp
  end
end
search = Proc.new do |line|
  if /enum\s+_HTTP_VERB/ =~ line
    inp
  else  
    search
  end  
end
proc = search
File.open(ARGV[0]).each_line do |line|
  proc = proc.call(line)
end
