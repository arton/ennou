# coding: utf-8

require_relative './ennou.rb'

module Rack
  module Handler
    class Ennoumu < Ennou

      @qname = 'EnnouMu_Queue'
      @nprocs = 2
      @rackup = 'rackup'

      def self.config(options)
        set_option(:@qname, options[:qname])
        set_option(:@nprocs, options[:nprocs])
      end

      def self.set_option(v, s)
        instance_variable_set(v, s) if s
      end
        
      def self.run(app, options = {})
        setup(options)
        ::Ennou::Server.open(@qname, true) do |server|
          @server = server
          if server.controller?
            @stoprun = false
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} start on #{RUBY_VERSION}(#{RUBY_PLATFORM})"
            if @script == ''
              server.add "http://#{@host}:#{@port}/"
            else
              server.add "http://#{@host}:#{@port}#{@script}/"
            end
            pids = []
            cmd = "#{::File.expand_path('../ruby.exe', $0)} #{::File.expand_path("../#{@rackup}", $0)} #{$DEBUG ? '-d' : ''} #{$VERBOSE ? '-w' : ''} #{@host == '+' ? '' : "-o #{@host}"} -p #{@port} -s Ennoumu \"#{options[:config]}\""
            1.upto(@nprocs) do
              pids << spawn(cmd)
              @logger.info " spawn worker pid=#{pids.last}"
            end
            until @stoprun do
              sleep 1
            end  
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} stop"
          else
            server.script =  @script
            @logger.info "script=#{server.script}, #{@script}"
            @logger.info "Ennou(#{::Ennou::VERSION}) start for http://#{@host}:#{@port}#{@script} pid=#{$$}"
            loop do
              begin
                r = server.wait(60)
                next if r.nil?
                run_thread(app, *r)
              rescue Interrupt
                break
              end
            end
            @logger.info "Ennou(#{::Ennou::VERSION}) stop service for http://#{@host}:#{@port}#{@script} pid=#{$$}"
          end
        end
      end   

      def self.multiprocess?
        true
      end

      def self.shutdown
        if @server.controller?
          @stoprun = true
        else
          @server.break
          @logger.info "going to shutdown ... pid=#{$$}"
        end
      end
      
      private
      
    end
  end    
end
