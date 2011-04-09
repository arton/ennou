skip = Proc.new do |line|
  skip
end
inp = Proc.new do |line|
  if /^}/ =~ line
    skip
  else
    if /^\s+HttpHeader([^\s]+)/ =~ line
      puts(%!    "#{$1.gsub(/([A-Z])/) {  '_' + $1 }.upcase[1..-1]}",!)
    end  
    inp
  end
end
search = Proc.new do |line|
  if /_HTTP_HEADER_ID/ =~ line
    inp
  else  
    search
  end  
end
proc = search
File.open(ARGV[0]).each_line do |line|
  proc = proc.call(line)
end
