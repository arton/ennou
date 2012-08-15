# must keep require order for test/unit run prior ennou end.
require 'ennou'
require 'test/unit'
require 'open-uri'
require 'net/http'

class TestShutdown < Test::Unit::TestCase
  QNAME = "TEST_QUEUE"

  def test_break
    request_thread = nil
    tserver = Thread.start do
      server = Ennou::Server.open(QNAME)
      server.add 'http://+:9393/test'
      env, io = server.wait(1)
      Thread.start do
        sleep(5)
        io.lump 200, { 'content-type' => "text/plain" }, 'hello'
      end
      assert_equal 1, server.requests
      server.close
      assert_equal 0, server.requests          
    end
    Thread.pass
    request_thread = Thread.start do
      begin
      open('http://localhost:9393/test/', 'Accept' => 'text/html,*`/*') do |f|
        assert_equal 'text/plain', f.content_type
        assert_equal [], f.content_encoding
        assert_equal 'hello', f.read
      end
        rescue => e
        puts e.backtrace
        end
    end
    request_thread.join    
    tserver.join
  end
end
