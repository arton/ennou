# coding: utf-8
=begin
run test:
=end
# must keep require order for test/unit run prior ennou end.
require_relative '../lib/rack/handler/ennou'
require 'test/unit'
require 'net/http'
require 'stringio'

module Rack
  VERSION = '0.0.0 dummy'
end

class TestRack < Test::Unit::TestCase
  
  def setup
    @count = 0
    @server = Thread.start do
      begin
        Rack::Handler::Ennou.run(self, Port:'9393', Host:'0.0.0.0', config:__FILE__)
      rescue
        p $!
      end  
    end
  end
  
  def teardown
    Rack::Handler::Ennou.shutdown
    @server.join
  end
  
  def test_app
    1.upto(30) do |i|
      Net::HTTP.start('localhost', 9393) do |http|
        data = 'b' * (@count + 1)
        resp = http.post('/test', 'b' * (@count + 1), { 'content-type' => 'text/plain' })
        assert_equal(data, @reqbody)
        assert_equal('200', resp.code)
        assert_equal(@count, resp.content_length)
        assert_equal('a' * @count, resp.body)
      end        
    end
  end
  
  def call(env)
    req = env['rack.input'].read
    @reqbody = req
    @count += 1
    [200, { 'Content-Length' => @count.to_s }, StringIO.new('a' * @count)]
  end
  
end
