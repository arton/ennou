# must keep require order for test/unit run prior ennou end.
require 'ennou'
require 'test/unit'
require 'open-uri'
require 'net/http'

class TestServer < Test::Unit::TestCase
  QNAME = "TEST_QUEUE"
  def test_rii
    svr = nil
    Ennou::Server.open(QNAME) do |server|
      svr = server
      assert Numeric === svr.group_id      
    end
    assert svr.group_id.nil?    
  end

  def test_openclose
    svr = Ennou::Server.new(QNAME)
    assert Numeric === svr.group_id
    svr.close
    assert svr.group_id.nil?
  end
  
  def test_get_timeout
    tos = [120, 120, 130, 130, 120, 140]
    Ennou.timeout = tos
    0.upto(tos.size) do |i|
      assert_equal tos[i], Ennou.timeout[i]
    end
  end
  
  def test_addurl
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:8080/test/'
        # by administrator
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end
  
  def test_noarg
    begin
      Ennou::Server.open
      fail
    rescue ArgumentError => e  
      assert(/wrong number of arguments/ =~ e.message)
    end  
  end

  def test_badtype
    begin
      Ennou::Server.open(1)
      fail
    rescue TypeError => e
      assert(/expected String/ =~ e.message)
    end  
  end
  
  def test_cantinstantiate
    begin
      Ennou::EnnouIO.new
      fail
    rescue RuntimeError => e
      assert /can\'t instantiate/ =~ e.message
    end
  end

  def test_lump
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          open('http://localhost/test/', 'Accept' => 'text/html,*/*') do |f|
            assert_equal 'text/plain', f.content_type
            assert_equal [], f.content_encoding
            assert_equal 'hello', f.read
          end
        end
        env, io = s.wait(0.1)
        assert_equal("GET", env['REQUEST_METHOD'])
        assert_equal("HTTP/1.1", env['SERVER_PROTOCOL'])
        assert_equal("Ruby", env['HTTP_USER_AGENT'])
        assert_equal("/test", env['SCRIPT_NAME'])
        assert_equal("/", env['PATH_INFO'])
        assert_equal("", env['QUERY_STRING'])
        assert_equal("text/html,*/*", env['HTTP_ACCEPT'])
        io.lump 200, { 'content-type' => "text/plain" }, 'hello'
        t.join
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end

  def test_setprop
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          open('http://localhost/test/') do |f|
            assert_equal 'hello', f.read
          end
        end
        env, io = s.wait(0.1)
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.body = 'hello'
        t.join
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end


  def test_write
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          assert_equal "hello world !", open('http://localhost/test/').read
        end
        env, io = s.wait(0.1)
        io.status = 200
        io.headers = { 'content-type' => "text/plain", 'content-length' => '13' }
        io.write 'hell'
        io.write 'o '
        io.write 'world !'
        io.close
        t.join
      rescue SystemCallError => e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end

  def test_write_wo_contentlength
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          assert_equal "hello world !", open('http://localhost/test/').read
        end
        env, io = s.wait(0.1)
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.write 'hell'
        io.write 'o '
        io.write 'world !'
        io.close
        t.join
      rescue SystemCallError => e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end

  def test_close
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          assert_equal "hello", open('http://localhost/test/').read
        end
        env, io = s.wait(0.1)
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.write 'hello'
        io.close
        t.join
      rescue SystemCallError => e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end

  def test_disc
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          assert_equal "hello", open('http://localhost/test/').read
        end
        env, io = s.wait(0.1)
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.write 'hello'
        io.disconnect
        t.join
      rescue SystemCallError => e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end

  def test_uri
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/'
        # by administrator
        Thread.start do
          open('http://localhost/').read
        end
        env, io = s.wait(0.1)
        assert_equal("", env['SCRIPT_NAME'])
        assert_equal("/", env['PATH_INFO'])
        assert_equal("", env['QUERY_STRING'])
        Thread.start do
          open('http://localhost/test').read          
        end
        env, io = s.wait(0.1)
        assert_equal("/test", env['SCRIPT_NAME'])
        assert_equal("", env['PATH_INFO'])
        assert_equal("", env['QUERY_STRING'])
        Thread.start do
          open('http://localhost/test?a=3&b=4').read          
        end
        env, io = s.wait(0.1)
        assert_equal("/test", env['SCRIPT_NAME'])
        assert_equal("", env['PATH_INFO'])
        assert_equal("a=3&b=4", env['QUERY_STRING'])
        Thread.start do
          open('http://localhost/test/?a=3&b=4').read          
        end
        env, io = s.wait(0.1)
        assert_equal("/test", env['SCRIPT_NAME'])
        assert_equal("/", env['PATH_INFO'])
        assert_equal("a=3&b=4", env['QUERY_STRING'])
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end
  
  def test_too_long_uri
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/'
        # by administrator
        t = Thread.start do
          begin
            p open('http://localhost/' + ('x' * 2000)).read
            fail
          rescue => e
            assert /400 Bad Request/ =~ e.message
          end  
        end
        env, io = s.wait(0.5)
        assert_equal nil, env
        t.join
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end

  def test_nclients
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test'
        # by administrator
        ts = Thread.start do
          loop do
            env, io = s.wait(0.5)
            Thread.start do
              io.lump 200, { 'content-type' => "text/plain" }, 'hello'
            end
          end
        end
        a = []
        10.times do 
          a << Thread.start do
            assert_equal "hello", open('http://localhost/test/').read
          end  
        end
        a.each do |t|
          t.join
        end
        ts.kill
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end
  
  def test_small_post
    data = 'a' * 999999
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          Net::HTTP.start('localhost') do |http|
            resp = http.post('/test/post', data)
            assert_equal 'hello', resp.body
          end
        end
        env, io = s.wait(0.1)
        reqdata = io.input.read
        assert_equal data, reqdata
        assert Encoding::BINARY === reqdata.encoding        
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.write 'hello'
        io.close
        t.join
      rescue SystemCallError => e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end

  def test_n_small_posts
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test'
        # by administrator
        ts = Thread.start do
          loop do
            env, io = s.wait(0.5)
            Thread.start do
              data = io.input.read
              io.lump 200, { 'content-type' => "text/plain" }, data
            end
          end
        end
        a = []
        'a'.upto('k') do |c|
          a << Thread.start do
            Net::HTTP.start('localhost') do |http|
              data = c * 99999
              resp = http.post('/test/post', data)
              assert_equal data, resp.body
            end
          end
        end
        a.each do |t|
          t.join
        end
        ts.kill
      rescue SystemCallError => e
        assert(/\(5\)/ =~ e.message)
      end
    end
  end

  def test_large_post
    data = 'abc' * 444444
    Ennou::Server.open(QNAME) do |s|
      begin
        s.add 'http://+:80/test/'
        # by administrator
        t = Thread.start do
          Net::HTTP.start('localhost') do |http|
            resp = http.post('/test/post', data)
            assert_equal data, resp.body
          end
        end
        env, io = s.wait(0.1)
        reqdata = io.input.read
        assert Tempfile === io.input
        assert_equal data, reqdata
        assert Encoding::BINARY === reqdata.encoding
        
        io.status = 200
        io.headers = { 'content-type' => "text/plain" }
        io.write data
        io.close
        t.join
      rescue SystemCallError => e
        p e
        assert(e.message, /\(5\)/ =~ e.message)
      end
    end
  end
end
